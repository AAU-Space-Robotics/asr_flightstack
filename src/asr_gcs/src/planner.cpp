#include "planner.h"

#include <fstream>
#include <sstream>
#include <utility>

using namespace asr_mission;

namespace {

// This probably belongs in skills.yaml itself
nlohmann::json DefaultParamValue(const std::string &skill, const std::string &param_name,
                                 const ParamSpec &spec) {
    if (skill == "takeoff" && param_name == "alt") {
        return 1.5;  // metres, positive-up
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

} // namespace

Planner::Planner(rclcpp::Node *node)
    : node_(node)
{
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
    json issues = json::parse(data);
    bool has_error = false;
    if (issues.empty()) {
        std::cout << "Vehicle validation: OK, no issues\n";
    }
    for (const auto &issue : issues) {
        std::cout << "  [" << issue.at("severity").get<std::string>() << "] "
                  << issue.at("path").get<std::string>() << ": "
                  << issue.at("message").get<std::string>() << "\n";
        if (issue.at("severity") == "error") has_error = true;
    }
    if (has_error) {
        std::cout << "Plan has errors -- not starting\n";
        return;
    }
    start();
}

void Planner::on_status(const std::string &data) {
    json status = json::parse(data);
    std::string state = status.at("state").get<std::string>();
    std::cout << "  [" << state << "] " << status.at("active_path").get<std::string>() << "\n";
    if (state == "success" || state == "failure") {
        std::cout << "Plan finished: " << state << "\n";
    }
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
    plan_ = Plan{};  // a plan built for one vehicle isn't guaranteed to fit another's skills
}

const VehicleCapabilities *Planner::selected_capabilities() const
{
    return selected_capabilities_ ? &*selected_capabilities_ : nullptr;
}

void Planner::add_task(const std::string &skill, const nlohmann::json &params)
{
    if (!plan_.root) {
        plan_.root = std::make_unique<SequenceNode>();
    }
    auto *sequence = static_cast<SequenceNode *>(plan_.root.get());

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
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) { return; }
    auto *sequence = static_cast<SequenceNode *>(plan_.root.get());
    if (index >= sequence->children.size()) { return; }
    sequence->children.erase(sequence->children.begin() + static_cast<long>(index));
}

void Planner::move_task_up(size_t index)
{
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) { return; }
    auto *sequence = static_cast<SequenceNode *>(plan_.root.get());
    if (index == 0 || index >= sequence->children.size()) { return; }
    std::swap(sequence->children[index - 1], sequence->children[index]);
}

void Planner::move_task_down(size_t index)
{
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) { return; }
    auto *sequence = static_cast<SequenceNode *>(plan_.root.get());
    if (index + 1 >= sequence->children.size()) { return; }
    std::swap(sequence->children[index], sequence->children[index + 1]);
}

void Planner::set_task_param(size_t index, const std::string &param_name, const nlohmann::json &value)
{
    if (!plan_.root || plan_.root->kind() != NodeKind::Sequence) { return; }
    auto *sequence = static_cast<SequenceNode *>(plan_.root.get());
    if (index >= sequence->children.size() || sequence->children[index]->kind() != NodeKind::Task) { return; }
    auto *task = static_cast<TaskNode *>(sequence->children[index].get());
    task->params[param_name] = value;
}

void Planner::clear()
{
    plan_ = Plan{};
}

std::vector<Issue> Planner::local_issues() const
{
    return validate(plan_, selected_capabilities());
}

void Planner::save(const std::string &path) const
{
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
    } catch (const PlanFormatError &) {
    }
}

void Planner::upload()
{
    std_msgs::msg::String msg;
    msg.data = plan_.dump_canonical();
    upload_pub_->publish(msg);
}

void Planner::start()
{
    std_msgs::msg::String msg;
    msg.data = plan_.plan_id;
    start_pub_->publish(msg);
}

void Planner::abort()
{
    std_msgs::msg::String msg;
    msg.data = plan_.plan_id;
    abort_pub_->publish(msg);
}
