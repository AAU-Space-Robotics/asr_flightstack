#include "planner/planner.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <utility>

using namespace asr_mission;

namespace {

// This probably belongs in skills.yaml itself
nlohmann::json DefaultParamValue(const std::string &skill, const std::string &param_name,
                                 const ParamSpec &spec) {
    if (skill == "takeoff" && param_name == "alt") {
        return -1.5;  // metres, NED (up is negative)
    }
    if (spec.type == "bool") { return false; }
    if (spec.type == "string") { return std::string(); }
    if (spec.type == "point") { return nlohmann::json::array({0.0, 0.0, 0.0}); }
    if (spec.type == "polygon") { return nlohmann::json::array(); }
    double value = 0.0;
    if (spec.min && spec.max) { value = (*spec.min + *spec.max) / 2.0; }
    else if (spec.min) { value = *spec.min; }
    else if (spec.max) { value = *spec.max; }
    return spec.type == "int" ? nlohmann::json(static_cast<int>(value)) : nlohmann::json(value);
}

// mission_executor_node.cpp matches start/abort requests against this.
std::string GeneratePlanId() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    char buf[24];
    std::snprintf(buf, sizeof(buf), "plan-%016llx", static_cast<unsigned long long>(dist(rng)));
    return buf;
}

// Shared by wrap_in_run_until/repeat/retry -- splices valid `indices` out of `sequence` into a new SequenceNode.
std::unique_ptr<SequenceNode> ExtractContiguousTasks(SequenceNode &sequence, const std::vector<size_t> &indices,
                                                      size_t &out_insert_at) {
    if (indices.empty()) { return nullptr; }
    std::vector<size_t> sorted = indices;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (sorted[i] >= sequence.children.size()) { return nullptr; }
        if (i > 0 && sorted[i] != sorted[i - 1] + 1) { return nullptr; }  // must be contiguous
        if (sequence.children[sorted[i]]->kind() != NodeKind::Task) { return nullptr; }  // not already grouped
    }

    auto inner = std::make_unique<SequenceNode>();
    for (size_t idx : sorted) {
        inner->children.push_back(std::move(sequence.children[idx]));
    }
    sequence.children.erase(sequence.children.begin() + static_cast<long>(sorted.front()),
                             sequence.children.begin() + static_cast<long>(sorted.back()) + 1);
    out_insert_at = sorted.front();
    return inner;
}

} // namespace

Planner::Planner(rclcpp::Node *node)
    : node_(node)
{
    plan_.plan_id = GeneratePlanId();
    auto reliable = rclcpp::QoS(4).reliable();
    auto best_effort = rclcpp::QoS(1).best_effort();

    upload_pub_ = node_->create_publisher<std_msgs::msg::String>("in/mission_upload", reliable);
    start_pub_  = node_->create_publisher<std_msgs::msg::String>("in/mission_start", reliable);
    abort_pub_  = node_->create_publisher<std_msgs::msg::String>("in/mission_abort", reliable);

    validate_sub_ = node_->create_subscription<std_msgs::msg::String>(
        "out/mission_validate", reliable,
        [this](const std_msgs::msg::String::SharedPtr msg) { on_validate(msg->data); });
    status_sub_ = node_->create_subscription<std_msgs::msg::String>(
        "out/mission_status", best_effort,
        [this](const std_msgs::msg::String::SharedPtr msg) { on_status(msg->data); });
}

void Planner::on_validate(const std::string &data) {
    json parsed = json::parse(data);
    std::string plan_id = parsed.value("plan_id", std::string());
    json issues = parsed.value("issues", json::array());

    std::lock_guard<std::mutex> lock(state_mutex_);
    // Empty plan_id means an unparseable blob, accept it; non-empty and mismatched means a stale response.
    if (!plan_id.empty() && plan_id != pending_upload_plan_id_) {
        return;
    }
    if (upload_timeout_timer_) { upload_timeout_timer_->cancel(); }

    upload_issues_.clear();
    for (const auto &issue : issues) {
        Severity severity = issue.at("severity") == "error" ? Severity::Error : Severity::Warning;
        upload_issues_.push_back({severity, issue.at("path").get<std::string>(), issue.at("message").get<std::string>()});
    }
    upload_status_ = UploadStatus::Validated;
}

void Planner::on_status(const std::string &data) {
    json status = json::parse(data);
    std::string state = status.at("state").get<std::string>();

    std::lock_guard<std::mutex> lock(state_mutex_);
    if (state == "running") { mission_status_ = MissionStatus::Running; }
    else if (state == "success") { mission_status_ = MissionStatus::Success; }
    else if (state == "failure") { mission_status_ = MissionStatus::Failure; }
}

MissionStatus Planner::mission_status() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return mission_status_;
}

std::map<std::string, VehicleCapabilities> Planner::available_vehicles() const
{
    return discover_capabilities();
}

void Planner::select_vehicle(const std::string &name)
{
    if (selected_capabilities_ && selected_capabilities_->vehicle == name) {
        return;
    }

    auto all = discover_capabilities();
    auto it = all.find(name);
    if (it == all.end()) {
        return;
    }

    selected_capabilities_ = it->second;
    // Unsupported tasks surface as validator errors rather than being silently wiped.
    plan_.vehicle = name;
}

const VehicleCapabilities *Planner::selected_capabilities() const
{
    return selected_capabilities_ ? &*selected_capabilities_ : nullptr;
}

SequenceNode *Planner::sequence_root()
{
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) { return nullptr; }
    return static_cast<SequenceNode *>(plan_.root.get());
}

void Planner::add_task(const std::string &skill, const nlohmann::json &params)
{
    // Not just a null check -- a loaded plan's root could be a non-Sequence node, which would corrupt memory if cast below.
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) {
        plan_.root = std::make_unique<SequenceNode>();
    }
    auto *sequence = sequence_root();

    auto task = std::make_unique<TaskNode>();
    task->skill = skill;
    task->params = params;

    if (selected_capabilities_) {
        auto it = selected_capabilities_->skills.find(skill);
        if (it != selected_capabilities_->skills.end()) {
            for (const auto &[param_name, spec] : it->second.params) {
                if (!task->params.contains(param_name)) {
                    task->params[param_name] = DefaultParamValue(skill, param_name, spec);
                }
            }
        }
    }

    sequence->children.push_back(std::move(task));
}

void Planner::remove_task(size_t index)
{
    auto *sequence = sequence_root();
    if (!sequence || index >= sequence->children.size()) { return; }
    sequence->children.erase(sequence->children.begin() + static_cast<long>(index));
}

void Planner::move_task_up(size_t index)
{
    auto *sequence = sequence_root();
    if (!sequence || index == 0 || index >= sequence->children.size()) { return; }
    std::swap(sequence->children[index - 1], sequence->children[index]);
}

void Planner::move_task_down(size_t index)
{
    auto *sequence = sequence_root();
    if (!sequence || index + 1 >= sequence->children.size()) { return; }
    std::swap(sequence->children[index], sequence->children[index + 1]);
}

void Planner::set_task_param(size_t index, const std::string &param_name, const nlohmann::json &value)
{
    auto *sequence = sequence_root();
    if (!sequence || index >= sequence->children.size() || sequence->children[index]->kind() != NodeKind::Task) { return; }
    auto *task = static_cast<TaskNode *>(sequence->children[index].get());
    task->params[param_name] = value;
}

void Planner::wrap_in_run_until(const std::vector<size_t> &indices)
{
    auto *sequence = sequence_root();
    if (!sequence) { return; }
    size_t insert_at = 0;
    auto inner = ExtractContiguousTasks(*sequence, indices, insert_at);
    if (!inner) { return; }

    auto group = std::make_unique<RunUntilNode>();
    group->child = std::move(inner);
    if (selected_capabilities_ && !selected_capabilities_->conditions.empty()) {
        Condition c;
        c.cond = selected_capabilities_->conditions.front();
        group->conditions_any.push_back(c);
    }
    sequence->children.insert(sequence->children.begin() + static_cast<long>(insert_at), std::move(group));
}

void Planner::wrap_in_repeat(const std::vector<size_t> &indices, int count)
{
    auto *sequence = sequence_root();
    if (!sequence) { return; }
    size_t insert_at = 0;
    auto inner = ExtractContiguousTasks(*sequence, indices, insert_at);
    if (!inner) { return; }

    auto group = std::make_unique<RepeatNode>();
    group->count = std::max(1, count);
    group->child = std::move(inner);
    sequence->children.insert(sequence->children.begin() + static_cast<long>(insert_at), std::move(group));
}

void Planner::wrap_in_retry(const std::vector<size_t> &indices, int max_attempts)
{
    auto *sequence = sequence_root();
    if (!sequence) { return; }
    size_t insert_at = 0;
    auto inner = ExtractContiguousTasks(*sequence, indices, insert_at);
    if (!inner) { return; }

    auto group = std::make_unique<RetryNode>();
    group->max_attempts = std::max(1, max_attempts);
    group->child = std::move(inner);
    sequence->children.insert(sequence->children.begin() + static_cast<long>(insert_at), std::move(group));
}

void Planner::ungroup(size_t group_index)
{
    auto *sequence = sequence_root();
    if (!sequence || group_index >= sequence->children.size()) { return; }

    PlanNode *node = sequence->children[group_index].get();
    PlanNodePtr *child_slot = nullptr;
    switch (node->kind()) {
        case NodeKind::RunUntil: child_slot = &static_cast<RunUntilNode *>(node)->child; break;
        case NodeKind::Repeat:   child_slot = &static_cast<RepeatNode *>(node)->child; break;
        case NodeKind::Retry:    child_slot = &static_cast<RetryNode *>(node)->child; break;
        default: return;
    }
    if (!*child_slot || (*child_slot)->kind() != NodeKind::Sequence) { return; }

    // Extracted by move before erase() below destroys the wrapper (and its owned child).
    auto *inner = static_cast<SequenceNode *>(child_slot->get());
    std::vector<PlanNodePtr> extracted = std::move(inner->children);

    auto it = sequence->children.erase(sequence->children.begin() + static_cast<long>(group_index));
    for (auto &task : extracted) {
        it = sequence->children.insert(it, std::move(task));
        ++it;
    }
}

RunUntilNode *Planner::group_at(size_t group_index)
{
    auto *sequence = sequence_root();
    if (!sequence || group_index >= sequence->children.size()
        || sequence->children[group_index]->kind() != NodeKind::RunUntil) {
        return nullptr;
    }
    return static_cast<RunUntilNode *>(sequence->children[group_index].get());
}

SequenceNode *Planner::wrapper_child_at(size_t group_index)
{
    auto *sequence = sequence_root();
    if (!sequence || group_index >= sequence->children.size()) { return nullptr; }

    PlanNode *node = sequence->children[group_index].get();
    PlanNode *child = nullptr;
    switch (node->kind()) {
        case NodeKind::RunUntil: child = static_cast<RunUntilNode *>(node)->child.get(); break;
        case NodeKind::Repeat:   child = static_cast<RepeatNode *>(node)->child.get(); break;
        case NodeKind::Retry:    child = static_cast<RetryNode *>(node)->child.get(); break;
        default: return nullptr;
    }
    return (child && child->kind() == NodeKind::Sequence) ? static_cast<SequenceNode *>(child) : nullptr;
}

void Planner::add_group_condition(size_t group_index)
{
    auto *group = group_at(group_index);
    if (!group) { return; }
    Condition c;
    if (selected_capabilities_ && !selected_capabilities_->conditions.empty()) {
        c.cond = selected_capabilities_->conditions.front();
    }
    group->conditions_any.push_back(c);
}

void Planner::remove_group_condition(size_t group_index, size_t condition_index)
{
    auto *group = group_at(group_index);
    if (!group || condition_index >= group->conditions_any.size()) { return; }
    group->conditions_any.erase(group->conditions_any.begin() + static_cast<long>(condition_index));
}

void Planner::set_group_condition(size_t group_index, size_t condition_index, const Condition &condition)
{
    auto *group = group_at(group_index);
    if (!group || condition_index >= group->conditions_any.size()) { return; }
    group->conditions_any[condition_index] = condition;
}

void Planner::set_repeat_count(size_t group_index, int count)
{
    auto *sequence = sequence_root();
    if (!sequence || group_index >= sequence->children.size()
        || sequence->children[group_index]->kind() != NodeKind::Repeat) { return; }
    static_cast<RepeatNode *>(sequence->children[group_index].get())->count = std::max(1, count);
}

void Planner::set_retry_max_attempts(size_t group_index, int max_attempts)
{
    auto *sequence = sequence_root();
    if (!sequence || group_index >= sequence->children.size()
        || sequence->children[group_index]->kind() != NodeKind::Retry) { return; }
    static_cast<RetryNode *>(sequence->children[group_index].get())->max_attempts = std::max(1, max_attempts);
}

void Planner::set_group_task_param(size_t group_index, size_t task_index,
                                    const std::string &param_name, const nlohmann::json &value)
{
    SequenceNode *inner = wrapper_child_at(group_index);
    if (!inner || task_index >= inner->children.size() || inner->children[task_index]->kind() != NodeKind::Task) { return; }
    auto *task = static_cast<TaskNode *>(inner->children[task_index].get());
    task->params[param_name] = value;
}

void Planner::remove_group_task(size_t group_index, size_t task_index)
{
    SequenceNode *inner = wrapper_child_at(group_index);
    if (!inner || task_index >= inner->children.size()) { return; }
    inner->children.erase(inner->children.begin() + static_cast<long>(task_index));

    if (inner->children.empty()) {
        auto *sequence = sequence_root();
        if (sequence && group_index < sequence->children.size()) {
            sequence->children.erase(sequence->children.begin() + static_cast<long>(group_index));
        }
    }
}

void Planner::move_group_task_up(size_t group_index, size_t task_index)
{
    SequenceNode *inner = wrapper_child_at(group_index);
    if (!inner || task_index == 0 || task_index >= inner->children.size()) { return; }
    std::swap(inner->children[task_index - 1], inner->children[task_index]);
}

void Planner::move_group_task_down(size_t group_index, size_t task_index)
{
    SequenceNode *inner = wrapper_child_at(group_index);
    if (!inner || task_index + 1 >= inner->children.size()) { return; }
    std::swap(inner->children[task_index], inner->children[task_index + 1]);
}

void Planner::clear()
{
    plan_ = Plan{};
    plan_.plan_id = GeneratePlanId();
    // Cleared plan stays owned by whichever vehicle is currently selected.
    if (selected_capabilities_) {
        plan_.vehicle = selected_capabilities_->vehicle;
    }
    reset_upload_status();
}

void Planner::reset_upload_status()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (upload_timeout_timer_) { upload_timeout_timer_->cancel(); }
    upload_status_ = UploadStatus::Idle;
    upload_issues_.clear();
    pending_upload_plan_id_.clear();
}

std::vector<Issue> Planner::local_issues() const
{
    return validate(plan_, selected_capabilities());
}

bool Planner::has_tasks() const
{
    const PlanNode *root = plan_.root.get();
    if (!root || root->kind() != NodeKind::Sequence) { return false; }
    return !static_cast<const SequenceNode *>(root)->children.empty();
}

void Planner::save(const std::string &path) const
{
    if (!has_tasks()) { return; }
    std::ofstream f(path);
    f << plan_.to_json().dump(2);
}

void Planner::load(const std::string &path)
{
    std::ifstream f(path);
    std::ostringstream buf;
    buf << f.rdbuf();
    try {
        plan_ = Plan::from_json(buf.str());
        if (plan_.plan_id.empty()) {  // older save, from before plans got one
            plan_.plan_id = GeneratePlanId();
        }
        if (!plan_.vehicle.empty()) {
            select_vehicle(plan_.vehicle);  // no-op if not found, or already selected
        }
        reset_upload_status();
    } catch (const PlanFormatError &) {
    }
}

void Planner::upload()
{
    if (!has_tasks()) { return; }

    std_msgs::msg::String msg;
    msg.data = plan_.dump_canonical();
    upload_pub_->publish(msg);

    // Plan isn't copyable, so the snapshot is a round-trip through the exact bytes just sent.
    uploaded_plan_ = Plan::from_json(msg.data);

    std::lock_guard<std::mutex> lock(state_mutex_);
    if (upload_timeout_timer_) { upload_timeout_timer_->cancel(); }
    pending_upload_plan_id_ = plan_.plan_id;
    upload_status_ = UploadStatus::Uploading;
    upload_issues_.clear();

    // One-shot: cancels itself the moment it fires, whether or not it actually flips the status.
    upload_timeout_timer_ = node_->create_wall_timer(kUploadTimeout, [this]() {
        std::lock_guard<std::mutex> timer_lock(state_mutex_);
        upload_timeout_timer_->cancel();
        if (upload_status_ == UploadStatus::Uploading) {
            upload_status_ = UploadStatus::TimedOut;
        }
    });
}

UploadStatus Planner::upload_status() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return upload_status_;
}

std::vector<Issue> Planner::upload_issues() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return upload_issues_;
}

void Planner::start()
{
    // Targets uploaded_plan_'s id, not plan_'s, since plan_ may have moved on since the last upload().
    std_msgs::msg::String msg;
    msg.data = uploaded_plan_.plan_id;
    start_pub_->publish(msg);
}

void Planner::abort()
{
    std_msgs::msg::String msg;
    msg.data = uploaded_plan_.plan_id;
    abort_pub_->publish(msg);
}
