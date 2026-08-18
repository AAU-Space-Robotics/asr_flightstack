// CLI test harness for the mission upload/start/status pipeline -- validates locally before touching the network.
// Usage: ros2 run asr_mission mission_cli <plan_file.json> [--vehicle NAME] [--abort-after SECONDS]
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <nlohmann/json.hpp>

#include "asr_mission/plan.h"
#include "asr_mission/plan_validator.h"
#include "asr_mission/capabilities.h"

using namespace asr_mission;
using json = nlohmann::json;

namespace {

// Publishing before DDS discovers a subscriber silently drops the message -- spin until one is seen.
bool wait_for_subscriber(const rclcpp::Node::SharedPtr &node, rclcpp::PublisherBase::SharedPtr pub,
                         double timeout_sec = 5.0) {
    const auto start = std::chrono::steady_clock::now();
    while (pub->get_subscription_count() == 0) {
        rclcpp::spin_some(node);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > timeout_sec) {
            std::cerr << "WARNING: no subscriber on " << pub->get_topic_name()
                      << " after " << timeout_sec << "s\n";
            return false;
        }
    }
    return true;
}

} // namespace

class MissionCli : public rclcpp::Node {
public:
    MissionCli(std::string plan_id, std::string raw_blob, std::optional<double> abort_after)
    : rclcpp::Node("mission_cli"), plan_id_(std::move(plan_id)), raw_blob_(std::move(raw_blob)),
      abort_after_(abort_after)
    {
        auto reliable = rclcpp::QoS(4).reliable();
        auto best_effort = rclcpp::QoS(1).best_effort();

        upload_pub_ = create_publisher<std_msgs::msg::String>("in/mission_upload", reliable);
        start_pub_  = create_publisher<std_msgs::msg::String>("in/mission_start", reliable);
        abort_pub_  = create_publisher<std_msgs::msg::String>("in/mission_abort", reliable);

        validate_sub_ = create_subscription<std_msgs::msg::String>(
            "out/mission_validate", reliable,
            [this](const std_msgs::msg::String::SharedPtr msg) { on_validate(msg->data); });
        status_sub_ = create_subscription<std_msgs::msg::String>(
            "out/mission_status", best_effort,
            [this](const std_msgs::msg::String::SharedPtr msg) { on_status(msg->data); });
    }

    void upload() {
        if (!wait_for_subscriber(shared_from_this(), upload_pub_)) {
            std::cerr << "No mission_executor listening on in/mission_upload -- is comms_gcs/comms_uav up?\n";
            rclcpp::shutdown();
            return;
        }
        std_msgs::msg::String msg;
        msg.data = raw_blob_;
        upload_pub_->publish(msg);
        std::cout << "Uploaded '" << plan_id_ << "' (" << raw_blob_.size()
                  << " bytes) -- waiting for vehicle validation...\n";
    }

private:
    void on_validate(const std::string &data) {
        json issues = json::parse(data).value("issues", json::array());
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
            rclcpp::shutdown();
            return;
        }
        start();
    }

    void start() {
        wait_for_subscriber(shared_from_this(), start_pub_);
        std_msgs::msg::String msg;
        msg.data = plan_id_;
        start_pub_->publish(msg);
        std::cout << "Sent start for '" << plan_id_ << "'\n";

        if (abort_after_) {
            abort_timer_ = create_wall_timer(
                std::chrono::duration<double>(*abort_after_),
                [this]() { send_abort(); });
        }
    }

    void send_abort() {
        abort_timer_->cancel();  // wall timer repeats by default -- only want this once
        std_msgs::msg::String msg;
        msg.data = plan_id_;
        abort_pub_->publish(msg);
        std::cout << "Sent abort for '" << plan_id_ << "' (after " << *abort_after_ << "s)\n";
    }

    void on_status(const std::string &data) {
        json status = json::parse(data);
        std::string state = status.at("state").get<std::string>();
        std::cout << "  [" << state << "] " << status.at("active_path").get<std::string>() << "\n";
        if (state == "success" || state == "failure") {
            std::cout << "Plan finished: " << state << "\n";
            rclcpp::shutdown();
        }
    }

    std::string plan_id_;
    std::string raw_blob_;
    std::optional<double> abort_after_;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    upload_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    start_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    abort_pub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr validate_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
    rclcpp::TimerBase::SharedPtr abort_timer_;
};

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plan_file.json> [--vehicle NAME] [--abort-after SECONDS]\n";
        return 1;
    }

    std::string plan_file = argv[1];
    std::string vehicle;
    std::optional<double> abort_after;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--vehicle" && i + 1 < argc) {
            vehicle = argv[++i];
        } else if (arg == "--abort-after" && i + 1 < argc) {
            abort_after = std::stod(argv[++i]);
        }
    }

    std::ifstream f(plan_file);
    if (!f.is_open()) {
        std::cerr << "Could not open plan file: " << plan_file << "\n";
        return 1;
    }
    std::ostringstream buf;
    buf << f.rdbuf();
    const std::string raw = buf.str();

    Plan plan;
    try {
        plan = Plan::from_json(raw);
    } catch (const PlanFormatError &e) {
        std::cerr << "Plan parse error: " << e.what() << "\n";
        return 1;
    }

    // Local validation, entirely offline -- structural only unless --vehicle names an installed package.
    std::unique_ptr<VehicleCapabilities> capabilities;
    if (!vehicle.empty()) {
        auto all_caps = discover_capabilities();
        auto it = all_caps.find(vehicle);
        if (it != all_caps.end()) {
            capabilities = std::make_unique<VehicleCapabilities>(it->second);
            std::cout << "Loaded local capabilities for '" << vehicle << "'\n";
        } else {
            std::cout << "No local skills.yaml for '" << vehicle << "' -- structural checks only\n";
        }
    }

    auto issues = validate(plan, capabilities.get());
    if (issues.empty()) {
        std::cout << "Local validation: OK, no issues (no vehicle contacted)\n";
    } else {
        std::cout << "Local validation (no vehicle contacted):\n";
        for (const auto &issue : issues) {
            std::cout << "  " << issue.to_string() << "\n";
        }
    }
    if (has_errors(issues)) {
        std::cout << "Plan has errors locally -- not uploading.\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<MissionCli>(plan.plan_id, raw, abort_after);
    node->upload();
    if (rclcpp::ok()) {
        rclcpp::spin(node);
    }
    rclcpp::shutdown();
    return 0;
}
