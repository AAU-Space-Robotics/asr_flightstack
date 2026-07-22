#include <fstream>
#include <sstream>

#include <rclcpp/rclcpp.hpp>

#include "asr_mission/plan.h"
#include "asr_mission/plan_executor.h"
#include "asr_mission/plan_validator.h"
#include "asr_mission/capabilities.h"

namespace asr_mission {

// Fake skill handle: logs the skill name, returns Success after one tick.
class FakeSkillHandle : public SkillHandle {
public:
    FakeSkillHandle(const std::string &skill, rclcpp::Logger logger)
        : skill_(skill), logger_(logger) {}

    Status poll() override {
        if (!started_) {
            RCLCPP_INFO(logger_, "[fake] skill '%s' started", skill_.c_str());
            started_ = true;
            return Status::Running;
        }
        RCLCPP_INFO(logger_, "[fake] skill '%s' complete", skill_.c_str());
        return Status::Success;
    }

    void cancel() override {
        RCLCPP_INFO(logger_, "[fake] skill '%s' cancelled", skill_.c_str());
    }

private:
    std::string skill_;
    rclcpp::Logger logger_;
    bool started_ = false;
};

class FakeSkillRunner : public SkillRunner {
public:
    explicit FakeSkillRunner(rclcpp::Logger logger) : logger_(logger) {}

    std::unique_ptr<SkillHandle> start(const std::string &skill,
                                       const nlohmann::json & /*params*/) override {
        return std::make_unique<FakeSkillHandle>(skill, logger_);
    }

private:
    rclcpp::Logger logger_;
};

// Fake condition source: all conditions evaluate to false (child runs to completion).
class FakeConditionSource : public ConditionSource {
public:
    bool evaluate(const Condition & /*condition*/) override { return false; }
};

class MissionExecutorNode : public rclcpp::Node {
public:
    MissionExecutorNode()
    : rclcpp::Node("mission_executor"),
      runner_(std::make_unique<FakeSkillRunner>(get_logger())),
      conditions_(std::make_unique<FakeConditionSource>())
    {
        declare_parameter<std::string>("plan_file", "");
        std::string plan_file = get_parameter("plan_file").as_string();

        if (plan_file.empty()) {
            RCLCPP_ERROR(get_logger(), "Parameter 'plan_file' is required");
            return;
        }

        // Load plan from file
        std::ifstream f(plan_file);
        if (!f.is_open()) {
            RCLCPP_ERROR(get_logger(), "Could not open plan file: %s", plan_file.c_str());
            return;
        }
        std::ostringstream buf;
        buf << f.rdbuf();

        try {
            plan_ = std::make_unique<Plan>(Plan::from_json(buf.str()));
        } catch (const PlanFormatError &e) {
            RCLCPP_ERROR(get_logger(), "Plan parse error: %s", e.what());
            return;
        }

        // Validate (structural only — no capabilities loaded yet)
        auto issues = validate(*plan_);
        for (const auto &issue : issues) {
            if (issue.severity == Severity::Error) {
                RCLCPP_ERROR(get_logger(), "Plan validation: %s", issue.to_string().c_str());
            } else {
                RCLCPP_WARN(get_logger(), "Plan validation: %s", issue.to_string().c_str());
            }
        }
        if (has_errors(issues)) {
            RCLCPP_ERROR(get_logger(), "Plan has errors — not starting executor");
            return;
        }

        RCLCPP_INFO(get_logger(), "Plan '%s' loaded and valid — starting executor",
                    plan_->plan_id.c_str());

        executor_ = std::make_unique<PlanExecutor>(*plan_, *runner_, *conditions_);

        // Tick at 2 Hz
        timer_ = create_wall_timer(std::chrono::milliseconds(500),
                                   [this]() { tick(); });
    }

private:
    void tick() {
        if (!executor_) { return; }

        Status status = executor_->tick();
        RCLCPP_DEBUG(get_logger(), "tick — path: %s", executor_->active_path().c_str());

        if (status == Status::Success) {
            RCLCPP_INFO(get_logger(), "Plan '%s' completed successfully", plan_->plan_id.c_str());
            timer_->cancel();
        } else if (status == Status::Failure) {
            RCLCPP_ERROR(get_logger(), "Plan '%s' failed", plan_->plan_id.c_str());
            timer_->cancel();
        }
    }

    std::unique_ptr<Plan> plan_;
    std::unique_ptr<FakeSkillRunner> runner_;
    std::unique_ptr<FakeConditionSource> conditions_;
    std::unique_ptr<PlanExecutor> executor_;
    rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace asr_mission

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<asr_mission::MissionExecutorNode>());
    rclcpp::shutdown();
    return 0;
}
