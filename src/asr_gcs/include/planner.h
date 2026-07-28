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
    void remove_task(size_t index);
    void move_task_up(size_t index);
    void move_task_down(size_t index);
    void set_task_param(size_t index, const std::string &param_name, const nlohmann::json &value);

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

    asr_mission::Plan plan_;
    std::optional<asr_mission::VehicleCapabilities> selected_capabilities_;

    // Guards state a ROS callback writes and the render loop reads.
    mutable std::mutex state_mutex_;
};
