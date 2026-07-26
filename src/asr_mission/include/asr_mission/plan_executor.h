// Tick-based plan interpreter.
//
// tick() advances the plan one step and returns immediately (Running/
// Success/Failure), so it can be driven from a ROS timer at mission rate
// (~1-10 Hz) without blocking -- a real skill like `goto` takes tens of
// seconds, so the executor must poll it rather than wait on it.
//
// Skills and conditions are injected through SkillRunner/ConditionSource, so
// PlanExecutor itself stays free of any DroneCommand/action-client specifics;
// the onboard node wires a real runner in, tests and GCS previews use fakes.
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

// A running skill instance (e.g. one in-flight DroneCommand goal).
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

// "time_elapsed" is handled entirely inside PlanExecutor (see RunUntilState)
// rather than routed here -- it means time since the enclosing run_until
// started, which is a property of the executor's own state, not something
// any external telemetry source could answer.
class ConditionSource {
public:
    virtual ~ConditionSource() = default;
    virtual bool evaluate(const Condition &condition) = 0;
};

bool compare(const std::string &op, double current, double value);

// Per-node runtime state (which child a sequence is on, a task's in-flight
// handle, a retry's attempt count, ...) is kept OUTSIDE the plan tree, keyed
// by node identity (PlanNode*), rather than as members on the nodes
// themselves -- so a Plan stays a plain, reusable, inspectable data
// structure even while it's running (e.g. a GUI can render the static tree
// without reaching into interpreter-owned state).
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

    // Per-node runtime state, kept outside the plan tree so the tree stays
    // a plain inspectable data structure while the executor runs it.
    struct TaskState     { std::unique_ptr<SkillHandle> handle; };
    struct SequenceState { size_t index = 0; };
    struct RetryState    { int attempts = 0; };
    // started_at is set on this run_until's first tick, so "time_elapsed"
    // measures time since THIS loop began, not since takeoff or plan start.
    struct RunUntilState { std::chrono::steady_clock::time_point started_at; };
    using NodeState = std::variant<TaskState, SequenceState, RetryState, RunUntilState>;

    std::unordered_map<const PlanNode *, NodeState> state_map_;
    std::string active_path_;

    Status tick_node(const PlanNode &node, const std::string &path);
    void reset_node(const PlanNode &node);
};

} // namespace asr_mission
