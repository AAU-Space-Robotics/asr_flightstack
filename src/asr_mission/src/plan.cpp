#include "asr_mission/plan.h"

namespace asr_mission {

json TaskNode::to_json() const {
    json j;
    j["type"] = "task";
    j["skill"] = skill;
    j["params"] = params;
    return j;
}

json SequenceNode::to_json() const {
    json j;
    j["type"] = "sequence";
    
    j["children"] = json::array();
    for (const auto &child : children) {
        j["children"].push_back(child->to_json());
    }

    return j;
}

json RetryNode::to_json() const {
    json j;

    j["type"] = "retry";
    j["max_attempts"] = max_attempts;
    j["child"] = child->to_json();

    return j;
}

json Condition::to_json() const {
    json j;
    j["cond"] = cond;

    if (op.has_value() && value.has_value()) {
        j["op"] = op.value();
        j["value"] = value.value();
    }

    return j;
}

Condition Condition::from_json(const json &j, const std::string &path) {
    if (!j.contains("cond")) {
        throw PlanFormatError(path, "condition missing 'cond'");
    }

    Condition c;
    c.cond = j.at("cond").get<std::string>();

    if (j.contains("op") && j.contains("value")) {
        c.op = j.at("op").get<std::string>();
        c.value = j.at("value").get<double>();
    }

    return c;
}

json RunUntilNode::to_json() const {
    json j;
    j["type"] = "run_until";

    j["conditions_any"] = json::array();
    for (const auto &condition : conditions_any) {
        j["conditions_any"].push_back(condition.to_json());
    }

    j["child"] = child->to_json();

    return j;
}

PlanNodePtr node_from_json(const json &j, const std::string &path) {
    if (!j.contains("type")) {
        throw PlanFormatError(path, "node missing 'type'");
    }

    std::string type = j.at("type").get<std::string>();
    if (type == "task") {
        if (!j.contains("skill")) {
            throw PlanFormatError(path, "task missing 'skill'");
        }
        auto node = std::make_unique<TaskNode>();
        node->skill = j.at("skill").get<std::string>();
        if (j.contains("params")) {
            node->params = j.at("params");
        }
        return node;
    } else if (type == "sequence") {
        auto node = std::make_unique<SequenceNode>();
        const json &raw_children = j.at("children");
        for (size_t i = 0; i < raw_children.size(); ++i) {
            PlanNodePtr child_node = node_from_json(raw_children.at(i), path + ".children[" + std::to_string(i) + "]");
            node->children.push_back(std::move(child_node));
        }
        return node;
    } else if (type == "retry") {

        if(!j.contains("max_attempts")){
            throw PlanFormatError(path, "retry missing 'max_attempts'");
        }
        
        auto node = std::make_unique<RetryNode>();
        node->max_attempts = j.at("max_attempts").get<int>();
        PlanNodePtr child = node_from_json(j.at("child"), path + ".child");
        node->child = std::move(child);
        
        return node;
    } else if (type == "run_until") {
        auto node = std::make_unique<RunUntilNode>();
        const json &conditions = j.at("conditions_any");
        for (size_t i = 0; i < conditions.size(); ++i) {
            node->conditions_any.push_back(Condition::from_json(conditions[i], path + ".conditions_any[" + std::to_string(i) + "]"));
        }
        PlanNodePtr child = node_from_json(j.at("child"), path + ".child");
        node->child = std::move(child);
  
        return node;
    } else {
        throw PlanFormatError(path, "unknown node type: " + type);
    }
}

json Plan::to_json() const {
    json j;
    j["plan_id"] = plan_id;
    j["schema_version"] = schema_version;
    j["root"] = root->to_json();
    return j;
}

std::string Plan::dump_canonical() const {
    return to_json().dump();
}

Plan Plan::from_json(const json &j) {
    if (!j.contains("root")) {
        throw PlanFormatError("plan", "missing 'root'");
    }

    Plan plan;
    if (j.contains("plan_id")) {
        plan.plan_id = j.at("plan_id").get<std::string>();
    }
    if (j.contains("schema_version")) {
        plan.schema_version = j.at("schema_version").get<int>();
    }
    plan.root = node_from_json(j.at("root"), "root");
    return plan;
}

Plan Plan::from_json(const std::string &blob) {
    json parsed;
    try {
        parsed = json::parse(blob);
    } catch (const json::parse_error &e) {
        throw PlanFormatError("plan", std::string("invalid JSON: ") + e.what());
    }
    return from_json(parsed);
}

} // namespace asr_mission
