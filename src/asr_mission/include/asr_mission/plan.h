// Mission plan data model, shared verbatim by the GCS and the onboard executor.
#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace asr_mission {

using json = nlohmann::json;

constexpr int kSchemaVersion = 1;

// Raised when a plan blob cannot be parsed into the data model.
class PlanFormatError : public std::runtime_error {
public:
    PlanFormatError(std::string path, const std::string &message)
        : std::runtime_error(path + ": " + message), path(std::move(path)) {}

    std::string path;
};

// Tags a PlanNode's concrete type so callers can switch instead of dynamic_cast.
enum class NodeKind { Task, Sequence, Retry, RunUntil, Repeat };

class PlanNode {
public:
    virtual ~PlanNode() = default;
    virtual NodeKind kind() const = 0;
    virtual json to_json() const = 0;
};

using PlanNodePtr = std::unique_ptr<PlanNode>;

class TaskNode : public PlanNode {
public:
    std::string skill;
    // Raw JSON object rather than a bespoke variant type -- simplest way to hold a small dynamic value.
    json params = json::object();

    NodeKind kind() const override { return NodeKind::Task; }
    json to_json() const override;
};

class SequenceNode : public PlanNode {
public:
    std::vector<PlanNodePtr> children;

    NodeKind kind() const override { return NodeKind::Sequence; }
    json to_json() const override;
};

class RetryNode : public PlanNode {
public:
    int max_attempts = 1;
    PlanNodePtr child;

    NodeKind kind() const override { return NodeKind::Retry; }
    json to_json() const override;
};

// A named predicate evaluated onboard, e.g. probes_found >= 3 -- boolean predicates carry no op/value.
struct Condition {std::string cond; 
                  std::optional<std::string> op; 
                  std::optional<double> value;

    json to_json() const;
    static Condition from_json(const json &j, const std::string &path);
};

class RunUntilNode : public PlanNode {
public:
    std::vector<Condition> conditions_any;
    PlanNodePtr child;

    NodeKind kind() const override { return NodeKind::RunUntil; }
    json to_json() const override;
};

class RepeatNode : public PlanNode {
public:
    int count = 1;
    PlanNodePtr child;

    NodeKind kind() const override { return NodeKind::Repeat; }
    json to_json() const override;
};

// Recursively parses one node and its children; `path` is a breadcrumb for PlanFormatError messages.
PlanNodePtr node_from_json(const json &j, const std::string &path = "root");

class Plan {
public:
    std::string plan_id;
    int schema_version = kSchemaVersion;
    // Vehicle this plan was authored for -- empty if never assigned.
    std::string vehicle;
    PlanNodePtr root;

    json to_json() const;
    // Canonical encoding: what goes over the link and what gets hashed.
    std::string dump_canonical() const;

    static Plan from_json(const json &j);
    static Plan from_json(const std::string &blob);
};

} // namespace asr_mission
