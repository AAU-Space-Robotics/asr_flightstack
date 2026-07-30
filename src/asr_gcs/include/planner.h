#pragma once

// Non-visual core of the mission planner. Owns the in-progress Plan, the
// selected vehicle's capabilities, and the ROS wiring to upload/validate/
// start/status/abort it against mission_executor. No ImGui calls belong
// here -- planner_panel.h/.cpp owns rendering.

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

class Planner {
public:
    // node is not owned -- publishers/subscriptions attach to the caller's
    // existing node.
    explicit Planner(rclcpp::Node *node);

    std::map<std::string, asr_mission::VehicleCapabilities> available_vehicles() const;

    void select_vehicle(const std::string &name);
    const asr_mission::VehicleCapabilities *selected_capabilities() const;

    void add_task(const std::string &skill, const nlohmann::json &params = nlohmann::json::object());

    // No-ops on an out-of-range index or a root that isn't a SequenceNode.
    // Applies to top-level entries regardless of kind, so a run_until group
    // moves/removes as a single unit just like a task does.
    void remove_task(size_t index);
    void move_task_up(size_t index);
    void move_task_down(size_t index);
    void set_task_param(size_t index, const std::string &param_name, const nlohmann::json &value);

    // Each splices the top-level tasks at `indices` (which must be
    // contiguous, ascending, and plain tasks -- not already grouped) into a
    // new node of the given kind, in their place. No-op if the selection
    // doesn't qualify.
    void wrap_in_run_until(const std::vector<size_t> &indices);
    void wrap_in_repeat(const std::vector<size_t> &indices, int count);
    void wrap_in_retry(const std::vector<size_t> &indices, int max_attempts);
    // Inverse of any of the three: replaces the group at `group_index` with
    // its child tasks, spliced back into the top level in their original
    // order. No-op if group_index isn't one of run_until/repeat/retry.
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
    // If removing task_index would leave the group with no tasks at all,
    // the whole group is removed too (a run_until/repeat/retry wrapping
    // nothing doesn't mean anything) -- so group_index may no longer name
    // anything after this call.
    void remove_group_task(size_t group_index, size_t task_index);
    void move_group_task_up(size_t group_index, size_t task_index);
    void move_group_task_down(size_t group_index, size_t task_index);

    void clear();

    std::vector<asr_mission::Issue> local_issues() const;

    const asr_mission::Plan &plan() const { return plan_; }

    void save(const std::string &path) const;
    void load(const std::string &path);

    void upload();
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

    // nullptr if group_index is out of range or not a run_until node.
    asr_mission::RunUntilNode *group_at(size_t group_index);
    // nullptr unless group_index names a run_until/repeat/retry node whose
    // child is itself a Sequence -- true for every such node this class
    // creates via wrap_in_*, but not guaranteed for a hand-authored plan.
    asr_mission::SequenceNode *wrapper_child_at(size_t group_index);
    // The plan's root, cast down, or nullptr if it's absent or not a
    // Sequence (e.g. a loaded plan whose root is some other node kind).
    asr_mission::SequenceNode *sequence_root();

    asr_mission::Plan plan_;
    std::optional<asr_mission::VehicleCapabilities> selected_capabilities_;

    // Guards state a ROS callback writes and the render loop reads.
    mutable std::mutex state_mutex_;
};
