#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <std_msgs/msg/string.hpp>
#include <asr_comms/action/uav_command.hpp>
#include <asr_comms/msg/telemetry_status.hpp>
#include <asr_comms/msg/telemetry_battery.hpp>

#include "asr_mission/plan.h"
#include "asr_mission/plan_executor.h"
#include "asr_mission/plan_validator.h"
#include "asr_mission/capabilities.h"

namespace asr_mission {

using UAVCommand           = asr_comms::action::UAVCommand;
using GoalHandleUAVCommand = rclcpp_action::ClientGoalHandle<UAVCommand>;

// One in-flight UAVCommand goal; callback-facing state lives in a separate shared_ptr CallbackState since this handle can be destroyed (by reset_node()) before cancel()'s async result_callback fires.
class UAVCommandSkillHandle : public SkillHandle {
public:
    UAVCommandSkillHandle(rclcpp_action::Client<UAVCommand>::SharedPtr client,
                            const UAVCommand::Goal &goal, rclcpp::Logger logger)
        : client_(client), state_(std::make_shared<CallbackState>())
    {
        if (!client_->action_server_is_ready()) {
            RCLCPP_ERROR(logger, "UAVCommand action server not ready — skill '%s' failed",
                        goal.command_type.c_str());
            state_->done = true;
            state_->succeeded = false;
            return;
        }

        auto state = state_;  // copied into the lambdas below, not `this`
        rclcpp_action::Client<UAVCommand>::SendGoalOptions opts;
        opts.goal_response_callback = [state, logger](const GoalHandleUAVCommand::SharedPtr &handle) {
            if (!handle) {
                RCLCPP_ERROR(logger, "UAVCommand goal rejected");
                state->done = true;
                state->succeeded = false;
            } else {
                state->goal_handle = handle;
            }
        };
        opts.result_callback = [state, logger](const GoalHandleUAVCommand::WrappedResult &result) {
            state->succeeded = (result.code == rclcpp_action::ResultCode::SUCCEEDED) &&
                               result.result->success;
            if (!state->succeeded) {
                const char *message = result.result ? result.result->message.c_str() : "no result";
                if (result.code == rclcpp_action::ResultCode::CANCELED) {
                    // Expected when a run_until/abort cancels an in-flight skill on purpose -- not a real failure.
                    RCLCPP_INFO(logger, "UAVCommand cancelled: %s", message);
                } else {
                    RCLCPP_ERROR(logger, "UAVCommand failed: %s", message);
                }
            }
            state->done = true;
        };
        client_->async_send_goal(goal, opts);
    }

    Status poll() override {
        if (!state_->done) return Status::Running;
        return state_->succeeded ? Status::Success : Status::Failure;
    }

    void cancel() override {
        if (state_->goal_handle && !state_->done) {
            client_->async_cancel_goal(state_->goal_handle);
        }
    }

private:
    struct CallbackState {
        GoalHandleUAVCommand::SharedPtr goal_handle;
        bool done = false;
        bool succeeded = false;
    };

    rclcpp_action::Client<UAVCommand>::SharedPtr client_;
    std::shared_ptr<CallbackState> state_;
    bool done_ = false;
    bool succeeded_ = false;
};

// Evaluates conditions against live telemetry; also tracks arming_state for UAVCommandSkillRunner.
class TelemetryConditionSource : public ConditionSource {
public:
    explicit TelemetryConditionSource(rclcpp::Node *node) {
        status_sub_ = node->create_subscription<asr_comms::msg::TelemetryStatus>(
            "out/telemetry/status", rclcpp::QoS(1).best_effort(),
            [this](const asr_comms::msg::TelemetryStatus::SharedPtr msg) {
                probes_found_ = msg->probes_found;
                arming_state_ = msg->arming_state;
            });
        battery_sub_ = node->create_subscription<asr_comms::msg::TelemetryBattery>(
            "out/telemetry/battery", rclcpp::QoS(1).best_effort(),
            [this](const asr_comms::msg::TelemetryBattery::SharedPtr msg) {
                battery_percent_ = msg->percentage;
            });
    }

    // "time_elapsed" never reaches here -- PlanExecutor handles it itself (see RunUntilState).
    bool evaluate(const Condition &condition) override {
        if (condition.cond == "probes_found") {
            return has_op_value(condition) &&
                   compare(*condition.op, static_cast<double>(probes_found_), *condition.value);
        }
        if (condition.cond == "battery_low") {
            return has_op_value(condition) &&
                   compare(*condition.op, static_cast<double>(battery_percent_), *condition.value);
        }
        // No search_grid skill exists yet, so nothing produces this signal -- never fire rather than guess.
        return false;
    }

    // Mirrors asr_autopilot's ArmingState::ARMED (state_manager.h).
    bool is_armed() const { return arming_state_ == kArmingStateArmed; }

private:
    static constexpr uint8_t kArmingStateArmed = 1;

    static bool has_op_value(const Condition &c) { return c.op.has_value() && c.value.has_value(); }

    rclcpp::Subscription<asr_comms::msg::TelemetryStatus>::SharedPtr  status_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_;
    int32_t probes_found_{0};
    float   battery_percent_{100.0f};  // optimistic default until the first real sample arrives
    uint8_t arming_state_{0};          // 0 = DISARMED, matches ArmingState::DISARMED
};

// Arms the vehicle first, then runs the real command, so a plan can skip an explicit "arm" step.
class AutoArmSkillHandle : public SkillHandle {
public:
    AutoArmSkillHandle(rclcpp_action::Client<UAVCommand>::SharedPtr client,
                       const UAVCommand::Goal &goal, rclcpp::Logger logger)
        : client_(client), goal_(goal), logger_(logger)
    {
        UAVCommand::Goal arm_goal;
        arm_goal.command_type = "arm";
        arming_ = std::make_unique<UAVCommandSkillHandle>(client_, arm_goal, logger_);
    }

    Status poll() override {
        if (arming_) {
            Status arm_status = arming_->poll();
            if (arm_status != Status::Success) { return arm_status; }
            arming_.reset();
            command_ = std::make_unique<UAVCommandSkillHandle>(client_, goal_, logger_);
        }
        return command_->poll();
    }

    void cancel() override {
        if (arming_) { arming_->cancel(); }
        else if (command_) { command_->cancel(); }
    }

private:
    rclcpp_action::Client<UAVCommand>::SharedPtr client_;
    UAVCommand::Goal goal_;
    rclcpp::Logger logger_;
    std::unique_ptr<UAVCommandSkillHandle> arming_;
    std::unique_ptr<UAVCommandSkillHandle> command_;
};

// Translates a skill+params pair into a UAVCommand goal, kept out of PlanExecutor itself.
class UAVCommandSkillRunner : public SkillRunner {
public:
    UAVCommandSkillRunner(rclcpp::Node *node, rclcpp::Logger logger,
                            const TelemetryConditionSource &telemetry)
        : logger_(logger), telemetry_(telemetry)
    {
        client_ = rclcpp_action::create_client<UAVCommand>(node, "in/uav_command");
    }

    std::unique_ptr<SkillHandle> start(const std::string &skill,
                                       const nlohmann::json &params) override {
        UAVCommand::Goal goal;
        goal.command_type = skill;
        bool needs_armed = false;

        if (skill == "takeoff") {
            // Plans express altitude as positive-up; asr_autopilot works in NED, where up is negative Z.
            goal.target_pose = {-params.at("alt").get<double>()};
            needs_armed = true;
        } else if (skill == "goto") {
            auto pos = params.at("pos").get<std::vector<double>>();
            if (pos.size() == 3) pos[2] = -pos[2];  // same positive-up -> NED conversion
            goal.target_pose = pos;
            if (params.contains("yaw")) {
                goal.yaw = params.at("yaw").get<double>();
            }
            needs_armed = true;
        } else if (skill == "spin") {
            // target_pose = [target_yaw, num_rotations, use_longest_path] -- see executeSpin().
            const bool longest = params.contains("longest_path") &&
                                 params.at("longest_path").get<bool>();
            goal.target_pose = {
                params.at("yaw").get<double>(),
                params.at("rotations").get<double>(),
                longest ? 1.0 : 0.0,
            };
            needs_armed = true;
        } else if (skill == "rth" || skill == "rtl") {
            needs_armed = true;
        }
        // land and other no-param skills: command_type alone is enough.

        if (needs_armed && !telemetry_.is_armed()) {
            return std::make_unique<AutoArmSkillHandle>(client_, goal, logger_);
        }
        return std::make_unique<UAVCommandSkillHandle>(client_, goal, logger_);
    }

private:
    rclcpp_action::Client<UAVCommand>::SharedPtr client_;
    rclcpp::Logger logger_;
    const TelemetryConditionSource &telemetry_;
};

class MissionExecutorNode : public rclcpp::Node {
public:
    MissionExecutorNode()
    : rclcpp::Node("mission_executor")
    {
        conditions_ = std::make_unique<TelemetryConditionSource>(this);
        runner_     = std::make_unique<UAVCommandSkillRunner>(this, get_logger(), *conditions_);

        declare_parameter<std::string>("vehicle", "");
        const std::string vehicle = get_parameter("vehicle").as_string();

        auto all_caps = discover_capabilities();
        if (!vehicle.empty()) {
            auto it = all_caps.find(vehicle);
            if (it != all_caps.end()) {
                capabilities_ = std::make_unique<VehicleCapabilities>(it->second);
                RCLCPP_INFO(get_logger(), "Loaded capabilities for vehicle '%s' (hash %s)",
                            vehicle.c_str(), capabilities_->hash().c_str());
            } else {
                RCLCPP_WARN(get_logger(), "No skills.yaml found for vehicle '%s' — "
                            "validating structurally only", vehicle.c_str());
            }
        } else {
            RCLCPP_WARN(get_logger(), "Parameter 'vehicle' not set — validating structurally only");
        }

        mission_upload_sub_ = create_subscription<std_msgs::msg::String>(
            "in/mission_upload", rclcpp::QoS(4).reliable(),
            [this](const std_msgs::msg::String::SharedPtr msg) { on_upload(msg->data); });

        mission_start_sub_ = create_subscription<std_msgs::msg::String>(
            "in/mission_start", rclcpp::QoS(4).reliable(),
            [this](const std_msgs::msg::String::SharedPtr msg) { on_start(msg->data); });

        mission_abort_sub_ = create_subscription<std_msgs::msg::String>(
            "in/mission_abort", rclcpp::QoS(4).reliable(),
            [this](const std_msgs::msg::String::SharedPtr msg) { on_abort(msg->data); });

        mission_validate_pub_ = create_publisher<std_msgs::msg::String>(
            "out/mission_validate", rclcpp::QoS(4).reliable());
        mission_status_pub_ = create_publisher<std_msgs::msg::String>(
            "out/mission_status", rclcpp::QoS(1).best_effort());

        RCLCPP_INFO(get_logger(), "mission_executor ready — waiting for a plan upload");
    }

private:
    void on_upload(const std::string &blob) {
        if (executor_ && status_ == Status::Running) {
            RCLCPP_WARN(get_logger(), "Upload rejected — a plan is already running");
            return;
        }

        Plan plan;
        try {
            plan = Plan::from_json(blob);
        } catch (const PlanFormatError &e) {
            json result = json::array();
            result.push_back({{"severity", "error"}, {"path", "plan"}, {"message", e.what()}});
            // No plan_id -- the blob didn't even parse, so there's nothing to echo.
            publish_validate("", result);
            RCLCPP_ERROR(get_logger(), "Plan parse error: %s", e.what());
            return;
        }

        auto issues = validate(plan, capabilities_.get());

        json result = json::array();
        for (const auto &issue : issues) {
            result.push_back({
                {"severity", issue.severity == Severity::Error ? "error" : "warning"},
                {"path", issue.path},
                {"message", issue.message},
            });
            if (issue.severity == Severity::Error) {
                RCLCPP_ERROR(get_logger(), "Plan validation: %s", issue.to_string().c_str());
            } else {
                RCLCPP_WARN(get_logger(), "Plan validation: %s", issue.to_string().c_str());
            }
        }
        publish_validate(plan.plan_id, result);

        if (has_errors(issues)) {
            RCLCPP_ERROR(get_logger(), "Plan '%s' has errors — not staged for start", plan.plan_id.c_str());
            pending_plan_.reset();
            return;
        }

        pending_plan_ = std::make_unique<Plan>(std::move(plan));
        RCLCPP_INFO(get_logger(), "Plan '%s' valid — staged, waiting for start",
                    pending_plan_->plan_id.c_str());
    }

    void on_start(const std::string &plan_id) {
        if (!pending_plan_) {
            RCLCPP_WARN(get_logger(), "Start rejected — no plan staged");
            return;
        }
        if (pending_plan_->plan_id != plan_id) {
            RCLCPP_WARN(get_logger(), "Start rejected — plan_id mismatch ('%s' staged, '%s' requested)",
                        pending_plan_->plan_id.c_str(), plan_id.c_str());
            return;
        }

        plan_     = std::move(pending_plan_);
        executor_ = std::make_unique<PlanExecutor>(*plan_, *runner_, *conditions_);
        status_   = Status::Running;

        RCLCPP_INFO(get_logger(), "Plan '%s' starting", plan_->plan_id.c_str());
        timer_ = create_wall_timer(std::chrono::milliseconds(500), [this]() { tick(); });
    }

    void on_abort(const std::string &plan_id) {
        if (!executor_ || status_ != Status::Running) {
            RCLCPP_WARN(get_logger(), "Abort rejected — no plan running");
            return;
        }
        if (plan_->plan_id != plan_id) {
            RCLCPP_WARN(get_logger(), "Abort rejected — plan_id mismatch ('%s' running, '%s' requested)",
                        plan_->plan_id.c_str(), plan_id.c_str());
            return;
        }

        RCLCPP_WARN(get_logger(), "Plan '%s' abort requested", plan_->plan_id.c_str());
        executor_->abort();
        tick();  // publish the resulting failure status now, don't wait for the next scheduled tick
    }

    void tick() {
        status_ = executor_->tick();
        RCLCPP_DEBUG(get_logger(), "tick — path: %s", executor_->active_path().c_str());
        publish_status();

        if (status_ != Status::Running) {
            if (status_ == Status::Success) {
                RCLCPP_INFO(get_logger(), "Plan '%s' completed successfully", plan_->plan_id.c_str());
            } else {
                RCLCPP_ERROR(get_logger(), "Plan '%s' failed", plan_->plan_id.c_str());
            }
            timer_->cancel();
        }
    }

    void publish_validate(const std::string &plan_id, const json &issues) {
        std_msgs::msg::String out;
        json envelope;
        envelope["plan_id"] = plan_id;
        envelope["issues"] = issues;
        out.data = envelope.dump();
        mission_validate_pub_->publish(out);
    }

    void publish_status() {
        json status_json;
        status_json["plan_id"]     = plan_->plan_id;
        status_json["active_path"] = executor_->active_path();
        status_json["state"] = (status_ == Status::Running) ? "running"
                              : (status_ == Status::Success) ? "success"
                                                              : "failure";
        std_msgs::msg::String out;
        out.data = status_json.dump();
        mission_status_pub_->publish(out);
    }

    std::unique_ptr<VehicleCapabilities> capabilities_;
    std::unique_ptr<Plan> pending_plan_;  // validated, not yet started
    std::unique_ptr<Plan> plan_;          // currently running
    std::unique_ptr<TelemetryConditionSource> conditions_;  // declared before runner_: runner_ holds a reference to it
    std::unique_ptr<SkillRunner> runner_;
    std::unique_ptr<PlanExecutor> executor_;
    Status status_ = Status::Success;  // idle state; no plan has failed or is running
    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_upload_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_start_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_abort_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    mission_validate_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    mission_status_pub_;
};

} // namespace asr_mission

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<asr_mission::MissionExecutorNode>());
    rclcpp::shutdown();
    return 0;
}
