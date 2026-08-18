#pragma once

// Non-visual core of the mission planner -- no ImGui calls belong here, planner_panel.h/.cpp owns rendering.

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "asr_mission/plan.h"
#include "asr_mission/plan_validator.h"
#include "asr_mission/capabilities.h"

enum class UploadStatus { Idle, Uploading, Validated, TimedOut };
enum class MissionStatus { Idle, Running, Success, Failure };

class Planner {
public:
    // node is not owned -- publishers/subscriptions attach to the caller's existing node.
    explicit Planner(rclcpp::Node *node);

    std::map<std::string, asr_mission::VehicleCapabilities> available_vehicles() const;

    void select_vehicle(const std::string &name);
    const asr_mission::VehicleCapabilities *selected_capabilities() const;

    void add_task(const std::string &skill, const nlohmann::json &params = nlohmann::json::object());

    // No-ops on an out-of-range index or a root that isn't a SequenceNode; a group moves/removes as one unit.
    void remove_task(size_t index);
    void move_task_up(size_t index);
    void move_task_down(size_t index);
    void set_task_param(size_t index, const std::string &param_name, const nlohmann::json &value);

    // Splices the top-level tasks at `indices` (contiguous, ascending, plain tasks only) into a new node.
    void wrap_in_run_until(const std::vector<size_t> &indices);
    void wrap_in_repeat(const std::vector<size_t> &indices, int count);
    void wrap_in_retry(const std::vector<size_t> &indices, int max_attempts);
    // Inverse of any of the three: splices the group's child tasks back into the top level in order.
    void ungroup(size_t group_index);

    // No-ops on an out-of-range group_index or one that isn't a run_until.
    void add_group_condition(size_t group_index);
    void remove_group_condition(size_t group_index, size_t condition_index);
    void set_group_condition(size_t group_index, size_t condition_index, const asr_mission::Condition &condition);
    // No-ops unless group_index names a repeat/retry node respectively.
    void set_repeat_count(size_t group_index, int count);
    void set_retry_max_attempts(size_t group_index, int max_attempts);
    // Works for a task nested under any of the three wrapper kinds.
    void set_group_task_param(size_t group_index, size_t task_index,
                               const std::string &param_name, const nlohmann::json &value);
    // If this empties the group, the whole group is removed too.
    void remove_group_task(size_t group_index, size_t task_index);
    void move_group_task_up(size_t group_index, size_t task_index);
    void move_group_task_down(size_t group_index, size_t task_index);

    void clear();

    std::vector<asr_mission::Issue> local_issues() const;

    const asr_mission::Plan &plan() const { return plan_; }

    // False for a fresh/cleared plan or one with no children -- save()/upload() no-op rather than crash.
    bool has_tasks() const;

    void save(const std::string &path);
    void load(const std::string &path);

    // Filename stem of the last save()/load(), regardless of which UI (Mission Control Bar or Planner tab) triggered it -- empty after clear().
    const std::string &plan_name() const { return plan_name_; }

    // Publishes the plan and tracks it: Uploading -> Validated (or TimedOut after kUploadTimeout); a new call supersedes any pending one.
    void upload();
    UploadStatus upload_status() const;
    // Only meaningful once upload_status() == Validated.
    std::vector<asr_mission::Issue> upload_issues() const;

    // Snapshot of the plan as actually sent -- start()/abort() target this, not plan(), which keeps changing as you edit.
    const asr_mission::Plan &uploaded_plan() const { return uploaded_plan_; }

    // Reflects the vehicle's own telemetry, unaffected by clear()/load() since those don't touch what's actually running.
    MissionStatus mission_status() const;

    void start();
    void abort();

private:
    rclcpp::Node *node_;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr upload_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr start_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr abort_pub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr validate_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;

    void on_validate(const std::string &data);
    void on_status(const std::string &data);

    // Cancels any pending upload timeout and drops back to Idle -- called whenever the plan is replaced wholesale.
    void reset_upload_status();

    // nullptr if group_index is out of range or not a run_until node.
    asr_mission::RunUntilNode *group_at(size_t group_index);
    // nullptr unless group_index names a wrapper node whose child is itself a Sequence.
    asr_mission::SequenceNode *wrapper_child_at(size_t group_index);
    // The plan's root, cast down, or nullptr if absent or not a Sequence.
    asr_mission::SequenceNode *sequence_root();

    asr_mission::Plan plan_;
    std::string plan_name_;
    std::optional<asr_mission::VehicleCapabilities> selected_capabilities_;

    // Guards state a ROS callback writes and the render loop reads.
    mutable std::mutex state_mutex_;

    // Guarded by state_mutex_: on_validate() runs on the spin thread, the rest run on the render thread.
    UploadStatus upload_status_ = UploadStatus::Idle;
    std::vector<asr_mission::Issue> upload_issues_;
    std::string pending_upload_plan_id_;
    rclcpp::TimerBase::SharedPtr upload_timeout_timer_;
    static constexpr std::chrono::milliseconds kUploadTimeout{15000};

    // Render-thread only (written by upload(), read by uploaded_plan()) -- no lock needed.
    asr_mission::Plan uploaded_plan_;

    // Guarded by state_mutex_: on_status() runs on the spin thread.
    MissionStatus mission_status_ = MissionStatus::Idle;
};
