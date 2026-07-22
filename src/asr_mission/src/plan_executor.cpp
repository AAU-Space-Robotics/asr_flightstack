#include "asr_mission/plan_executor.h"

namespace asr_mission {

bool compare(const std::string &op, double current, double value) {
    if (op == ">=") return current >= value;
    if (op == "<=") return current <= value;
    if (op == ">")  return current > value;
    if (op == "<")  return current < value;
    if (op == "==") return current == value;
    throw std::invalid_argument("unknown condition op: " + op);
}

PlanExecutor::PlanExecutor(const Plan &plan, SkillRunner &runner, ConditionSource &conditions)
    : plan_(plan), runner_(runner), conditions_(conditions) {}

Status PlanExecutor::tick() {
    if (status_ != Status::Running) { return status_; }
    status_ = tick_node(*plan_.root, "root");
    return status_;
}

void PlanExecutor::abort() {
    reset_node(*plan_.root);
    status_ = Status::Failure;
}

std::string PlanExecutor::active_path() const {
    return active_path_;
}

Status PlanExecutor::tick_node(const PlanNode &node, const std::string &path) {
    active_path_ = path;

    switch (node.kind()) {

        case NodeKind::Task: {
            const auto &task = static_cast<const TaskNode &>(node);
            if (!state_map_.count(&node)) { state_map_[&node] = TaskState{}; }
            auto &state = std::get<TaskState>(state_map_.at(&node));

            if (!state.handle) {
                state.handle = runner_.start(task.skill, task.params);
            }
            return state.handle->poll();
        }

        case NodeKind::Sequence: {
            const auto &seq = static_cast<const SequenceNode &>(node);
            if (!state_map_.count(&node)) { state_map_[&node] = SequenceState{}; }
            auto &state = std::get<SequenceState>(state_map_.at(&node));

            if (seq.children.empty()) { return Status::Success; }

            Status child_status = tick_node(
                *seq.children[state.index],
                path + ".children[" + std::to_string(state.index) + "]");

            if (child_status == Status::Success) {
                state.index++;
                return (state.index >= seq.children.size()) ? Status::Success : Status::Running;
            }
            return child_status;
        }

        case NodeKind::Retry: {
            const auto &retry = static_cast<const RetryNode &>(node);
            if (!state_map_.count(&node)) { state_map_[&node] = RetryState{}; }
            auto &state = std::get<RetryState>(state_map_.at(&node));

            Status child_status = tick_node(*retry.child, path + ".child");
            if (child_status != Status::Failure) { return child_status; }

            state.attempts++;
            if (state.attempts >= retry.max_attempts) { return Status::Failure; }
            reset_node(*retry.child);
            return Status::Running;
        }

        case NodeKind::RunUntil: {
            const auto &run_until = static_cast<const RunUntilNode &>(node);

            for (const auto &cond : run_until.conditions_any) {
                if (conditions_.evaluate(cond)) {
                    reset_node(*run_until.child);
                    return Status::Success;
                }
            }
            return tick_node(*run_until.child, path + ".child");
        }
    }

    return Status::Failure;
}

void PlanExecutor::reset_node(const PlanNode &node) {
    switch (node.kind()) {
        case NodeKind::Task: {
            auto it = state_map_.find(&node);
            if (it != state_map_.end()) {
                auto &state = std::get<TaskState>(it->second);
                if (state.handle) { state.handle->cancel(); }
                state_map_.erase(it);
            }
            break;
        }
        case NodeKind::Sequence: {
            const auto &seq = static_cast<const SequenceNode &>(node);
            for (const auto &child : seq.children) { reset_node(*child); }
            state_map_.erase(&node);
            break;
        }
        case NodeKind::Retry: {
            const auto &retry = static_cast<const RetryNode &>(node);
            reset_node(*retry.child);
            state_map_.erase(&node);
            break;
        }
        case NodeKind::RunUntil: {
            const auto &run_until = static_cast<const RunUntilNode &>(node);
            reset_node(*run_until.child);
            state_map_.erase(&node);
            break;
        }
    }
}

} // namespace asr_mission
