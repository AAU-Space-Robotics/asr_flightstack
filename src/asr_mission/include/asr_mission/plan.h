// Mission plan data model.
//
// Imported verbatim by the GCS (plan authoring/validation) and the onboard
// executor -- both link this library so the two sides can never drift. The
// wire format is the JSON produced by Plan::dump_canonical(); transports
// treat it as an opaque blob, so the encoding could later be swapped without
// touching transport framing.
//
// Node types:
//   task      - leaf, runs one skill from the vehicle's capabilities
//   sequence  - runs children in order, fails on first failure
//   retry     - re-runs its child up to max_attempts times on failure
//   run_until - runs its child, succeeds early if any condition becomes true;
//               otherwise finishes with the child's own terminal status
//   repeat    - re-runs its child count times unconditionally, but aborts
//               (fails) immediately if any run fails -- it never masks a
//               failure the way retry's "try again" does
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

// Tags a PlanNode's concrete type so the validator/executor can switch on it
// instead of chaining dynamic_cast.
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
    // Params kept as a raw JSON object rather than a bespoke variant type --
    // simplest way to hold a small dynamic value; the validator does runtime
    // type checks against it the same way it checks against a ParamSpec.
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

// A named predicate evaluated onboard, e.g. probes_found >= 3.
// Boolean predicates (grid_exhausted) carry no op/value.
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

// Recursively parses one node (and its children). `path` is a breadcrumb
// used in PlanFormatError messages, e.g. "root.children[2].child".
PlanNodePtr node_from_json(const json &j, const std::string &path = "root");

class Plan {
public:
    std::string plan_id;
    int schema_version = kSchemaVersion;
    PlanNodePtr root;

    json to_json() const;
    // Canonical encoding: what goes over the link and what gets hashed.
    std::string dump_canonical() const;

    static Plan from_json(const json &j);
    static Plan from_json(const std::string &blob);
};

} // namespace asr_mission
