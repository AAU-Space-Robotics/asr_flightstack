// Tick-based plan interpreter -- tick() advances one step and returns immediately, never blocking.
#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include <nlohmann/json.hpp>

#include "asr_mission/plan.h"

namespace asr_mission {

enum class Status { Running, Success, Failure };

// A running skill instance (e.g. one in-flight UAVCommand goal).
class SkillHandle {
public:
    virtual ~SkillHandle() = default;
    virtual Status poll() = 0;
    virtual void cancel() = 0;
};

class SkillRunner {
public:
    virtual ~SkillRunner() = default;
    virtual std::unique_ptr<SkillHandle> start(const std::string &skill, const nlohmann::json &params) = 0;
};

// "time_elapsed" is handled entirely inside PlanExecutor (see RunUntilState), not routed here.
class ConditionSource {
public:
    virtual ~ConditionSource() = default;
    virtual bool evaluate(const Condition &condition) = 0;
};

bool compare(const std::string &op, double current, double value);

// Per-node runtime state is kept OUTSIDE the plan tree, keyed by node identity, so a Plan stays plain/reusable.
class PlanExecutor {
public:
    PlanExecutor(const Plan &plan, SkillRunner &runner, ConditionSource &conditions);

    Status tick();
    // Operator abort: cancel anything running and stop the plan.
    void abort();
    // Where the program counter is, for status telemetry / GUI highlight.
    std::string active_path() const;

private:
    const Plan &plan_;
    SkillRunner &runner_;
    ConditionSource &conditions_;
    Status status_ = Status::Running;

    struct TaskState     { std::unique_ptr<SkillHandle> handle; };
    struct SequenceState { size_t index = 0; };
    struct RetryState    { int attempts = 0; };
    // started_at is set on this run_until's first tick, not takeoff or plan start.
    struct RunUntilState { std::chrono::steady_clock::time_point started_at; };
    struct RepeatState   { int completed = 0; };
    using NodeState = std::variant<TaskState, SequenceState, RetryState, RunUntilState, RepeatState>;

    std::unordered_map<const PlanNode *, NodeState> state_map_;
    std::string active_path_;

    Status tick_node(const PlanNode &node, const std::string &path);
    void reset_node(const PlanNode &node);
};

} // namespace asr_mission
