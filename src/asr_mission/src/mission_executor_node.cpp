#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <std_msgs/msg/string.hpp>
#include <asr_comms/action/drone_command.hpp>
#include <asr_comms/msg/telemetry_status.hpp>
#include <asr_comms/msg/telemetry_battery.hpp>

#include "asr_mission/plan.h"
#include "asr_mission/plan_executor.h"
#include "asr_mission/plan_validator.h"
#include "asr_mission/capabilities.h"

namespace asr_mission {

using DroneCommand           = asr_comms::action::DroneCommand;
using GoalHandleDroneCommand = rclcpp_action::ClientGoalHandle<DroneCommand>;

// One in-flight DroneCommand goal. Mirrors the goal_response/result_callback
// pattern comms_uav.cpp already uses to bridge this same action to mavlink.
//
// cancel() only *requests* an async cancel -- the actual result_callback
// fires later, whenever the server responds. reset_node() (plan_executor.cpp)
// calls cancel() and then immediately destroys this handle (erasing it from
// PlanExecutor's state map), so by the time that later callback fires, `this`
// would already be freed. The callback-facing fields live in a separate
// heap-allocated CallbackState, captured by shared_ptr (not raw `this`) in
// the lambdas, so they stay valid until the last owner -- this handle or the
// still-pending callback, whichever outlives the other -- lets go of it.
class DroneCommandSkillHandle : public SkillHandle {
public:
    DroneCommandSkillHandle(rclcpp_action::Client<DroneCommand>::SharedPtr client,
                            const DroneCommand::Goal &goal, rclcpp::Logger logger)
        : client_(client), state_(std::make_shared<CallbackState>())
    {
        if (!client_->action_server_is_ready()) {
            RCLCPP_ERROR(logger, "DroneCommand action server not ready — skill '%s' failed",
                        goal.command_type.c_str());
            state_->done = true;
            state_->succeeded = false;
            return;
        }

        auto state = state_;  // copied into the lambdas below, not `this`
        rclcpp_action::Client<DroneCommand>::SendGoalOptions opts;
        opts.goal_response_callback = [state, logger](const GoalHandleDroneCommand::SharedPtr &handle) {
            if (!handle) {
                RCLCPP_ERROR(logger, "DroneCommand goal rejected");
                state->done = true;
                state->succeeded = false;
            } else {
                state->goal_handle = handle;
            }
        };
        opts.result_callback = [state, logger](const GoalHandleDroneCommand::WrappedResult &result) {
            state->succeeded = (result.code == rclcpp_action::ResultCode::SUCCEEDED) &&
                               result.result->success;
            if (!state->succeeded) {
                const char *message = result.result ? result.result->message.c_str() : "no result";
                if (result.code == rclcpp_action::ResultCode::CANCELED) {
                    // Expected outcome when a run_until or mission abort
                    // cancels an in-flight skill on purpose -- not a real
                    // failure, so don't log it as one.
                    RCLCPP_INFO(logger, "DroneCommand cancelled: %s", message);
                } else {
                    RCLCPP_ERROR(logger, "DroneCommand failed: %s", message);
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
        GoalHandleDroneCommand::SharedPtr goal_handle;
        bool done = false;
        bool succeeded = false;
    };

    rclcpp_action::Client<DroneCommand>::SharedPtr client_;
    std::shared_ptr<CallbackState> state_;
    bool done_ = false;
    bool succeeded_ = false;
};

// Translates a skill+params pair (as declared in skills.yaml) into a
// DroneCommand goal. The mapping lives here rather than in PlanExecutor so
// the executor itself stays free of any DroneCommand-specific knowledge.
class DroneCommandSkillRunner : public SkillRunner {
public:
    DroneCommandSkillRunner(rclcpp::Node *node, rclcpp::Logger logger)
        : logger_(logger)
    {
        client_ = rclcpp_action::create_client<DroneCommand>(node, "in/drone_command");
    }

    std::unique_ptr<SkillHandle> start(const std::string &skill,
                                       const nlohmann::json &params) override {
        DroneCommand::Goal goal;
        goal.command_type = skill;

        if (skill == "takeoff") {
            // Plans express altitude as positive-up (skills.yaml: "alt");
            // asr_autopilot works in NED, where up is negative Z.
            goal.target_pose = {-params.at("alt").get<double>()};
        } else if (skill == "goto") {
            auto pos = params.at("pos").get<std::vector<double>>();
            if (pos.size() == 3) pos[2] = -pos[2];  // same positive-up -> NED conversion
            goal.target_pose = pos;
            if (params.contains("yaw")) {
                goal.yaw = params.at("yaw").get<double>();
            }
        }
        // land and other no-param skills: command_type alone is enough.

        return std::make_unique<DroneCommandSkillHandle>(client_, goal, logger_);
    }

private:
    rclcpp_action::Client<DroneCommand>::SharedPtr client_;
    rclcpp::Logger logger_;
};

// Evaluates conditions against live telemetry, published locally by
// asr_autopilot on this same vehicle-side ROS graph -- no comms bridge
// needed, mission_executor_node just subscribes directly like comms_uav
// does for the same topics.
class TelemetryConditionSource : public ConditionSource {
public:
    explicit TelemetryConditionSource(rclcpp::Node *node) {
        status_sub_ = node->create_subscription<asr_comms::msg::TelemetryStatus>(
            "out/telemetry/status", rclcpp::QoS(1).best_effort(),
            [this](const asr_comms::msg::TelemetryStatus::SharedPtr msg) {
                probes_found_ = msg->probes_found;
            });
        battery_sub_ = node->create_subscription<asr_comms::msg::TelemetryBattery>(
            "out/telemetry/battery", rclcpp::QoS(1).best_effort(),
            [this](const asr_comms::msg::TelemetryBattery::SharedPtr msg) {
                battery_percent_ = msg->percentage;
            });
    }

    // "time_elapsed" never reaches here -- PlanExecutor handles it itself
    // (see RunUntilState), since it means time since the enclosing
    // run_until started, not anything telemetry could answer.
    bool evaluate(const Condition &condition) override {
        if (condition.cond == "probes_found") {
            return has_op_value(condition) &&
                   compare(*condition.op, static_cast<double>(probes_found_), *condition.value);
        }
        if (condition.cond == "battery_low") {
            return has_op_value(condition) &&
                   compare(*condition.op, static_cast<double>(battery_percent_), *condition.value);
        }
        // No search_grid skill exists yet (see thyra's skills.yaml), so
        // nothing produces this signal -- never fire rather than guess.
        return false;
    }

private:
    static bool has_op_value(const Condition &c) { return c.op.has_value() && c.value.has_value(); }

    rclcpp::Subscription<asr_comms::msg::TelemetryStatus>::SharedPtr  status_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_;
    int32_t probes_found_{0};
    float   battery_percent_{100.0f};  // optimistic default until the first real sample arrives
};

class MissionExecutorNode : public rclcpp::Node {
public:
    MissionExecutorNode()
    : rclcpp::Node("mission_executor")
    {
        runner_     = std::make_unique<DroneCommandSkillRunner>(this, get_logger());
        conditions_ = std::make_unique<TelemetryConditionSource>(this);

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
            publish_validate(result);
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
        publish_validate(result);

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

    void publish_validate(const json &issues) {
        std_msgs::msg::String out;
        out.data = issues.dump();
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
    std::unique_ptr<SkillRunner> runner_;
    std::unique_ptr<ConditionSource> conditions_;
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
