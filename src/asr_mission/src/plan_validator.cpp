#include "asr_mission/plan_validator.h"
#include <algorithm>

namespace asr_mission {

std::string Issue::to_string() const {
    std::string prefix = (severity == Severity::Error) ? "[error] " : "[warning] ";
    return prefix + path + ": " + message;
}

// Does a JSON param value match a declared ParamSpec type? "point" is
// [x,y,z], "polygon" is [[x,y],...] -- see kParamTypes in capabilities.h.
static bool param_type_matches(const json &value, const std::string &type) {
    if (type == "float" || type == "int") return value.is_number();
    if (type == "bool") return value.is_boolean();
    if (type == "string") return value.is_string();
    if (type == "point") {
        return value.is_array() && value.size() == 3 &&
               std::all_of(value.begin(), value.end(), [](const json &e) { return e.is_number(); });
    }
    if (type == "polygon") {
        if (!value.is_array()) return false;
        for (const auto &vertex : value) {
            if (!vertex.is_array() || vertex.size() != 2 ||
                !vertex[0].is_number() || !vertex[1].is_number()) {
                return false;
            }
        }
        return true;
    }
    return false;  // unknown declared type -- fail closed
}

// Checks a task's params against its skill's declared ParamSpecs: required,
// type, and (for scalar float/int) min/max range. Not applied to point/
// polygon params -- no current skill declares min/max on those, and a
// per-component range isn't an obviously correct interpretation to guess at.
static void validate_params(const json &params, const SkillSpec &spec,
                            const std::string &path, std::vector<Issue> &issues) {
    for (const auto &[param_name, param_spec] : spec.params) {
        const std::string param_path = path + ".params." + param_name;

        if (!params.contains(param_name)) {
            if (param_spec.required) {
                issues.push_back({Severity::Error, param_path, "missing required param '" + param_name + "'"});
            }
            continue;
        }

        const json &value = params.at(param_name);
        if (!param_type_matches(value, param_spec.type)) {
            issues.push_back({Severity::Error, param_path,
                "param '" + param_name + "' should be type '" + param_spec.type + "'"});
            continue;
        }

        if ((param_spec.type == "float" || param_spec.type == "int")) {
            const double v = value.get<double>();
            if (param_spec.min.has_value() && v < *param_spec.min) {
                issues.push_back({Severity::Error, param_path,
                    "param '" + param_name + "' = " + std::to_string(v) + " below min " + std::to_string(*param_spec.min)});
            }
            if (param_spec.max.has_value() && v > *param_spec.max) {
                issues.push_back({Severity::Error, param_path,
                    "param '" + param_name + "' = " + std::to_string(v) + " above max " + std::to_string(*param_spec.max)});
            }
        }
    }
}

static void validate_node(const PlanNode &node, const std::string &path, const VehicleCapabilities *capabilities, std::vector<Issue> &issues) {
    
    switch (node.kind()){
        case NodeKind::Sequence: {
            const auto &sequence_node = static_cast<const SequenceNode&>(node);
            if (sequence_node.children.empty()) {
                issues.push_back({Severity::Error, path, "sequence node has no children"});
            }
            for (size_t i = 0; i < sequence_node.children.size(); ++i) {
                validate_node(*sequence_node.children[i], path + ".children[" + std::to_string(i) + "]", capabilities, issues);
            }
            break;
        }
        
        case NodeKind::Retry: {
            const auto &retry_node = static_cast<const RetryNode&>(node);
            
            // Validate the max_attempts
            if (retry_node.max_attempts < 1) {
                issues.push_back({Severity::Error, path, "retry node should have 1 or more max_attempts, not: " + std::to_string(retry_node.max_attempts)});
            }
            else if (retry_node.max_attempts > kMaxRetryAttempts) {
                issues.push_back({Severity::Warning, path, "retry node has more than " + std::to_string(kMaxRetryAttempts) + " max_attempts, which may be excessive: " + std::to_string(retry_node.max_attempts)});
            }
            
            validate_node(*retry_node.child, path + ".child", capabilities, issues);
            break;
        }
        
        case NodeKind::RunUntil: {
            const auto &run_until_node = static_cast<const RunUntilNode&>(node);
            if (run_until_node.conditions_any.empty()) {
                issues.push_back({Severity::Error, path, "run_until node has no conditions_any"});
            }

            if (capabilities != nullptr) {
                for (size_t i = 0; i < run_until_node.conditions_any.size(); ++i) {
                    const auto &condition = run_until_node.conditions_any[i];
                    if (std::find(capabilities->conditions.begin(), capabilities->conditions.end(), condition.cond) == capabilities->conditions.end()) {
                        issues.push_back({Severity::Error, path + ".conditions_any[" + std::to_string(i) + "]", "condition '" + condition.cond + "' not found in vehicle capabilities"});
                    }
                }
            }

            validate_node(*run_until_node.child, path + ".child", capabilities, issues);
            
            break;
        }

        case NodeKind::Task: {
            const auto &task_node = static_cast<const TaskNode&>(node);
            if (task_node.skill.empty()) {
                issues.push_back({Severity::Error, path, "task node has empty skill"});
            }

            if (capabilities != nullptr) {
                auto skill_it = capabilities->skills.find(task_node.skill);
                if (skill_it == capabilities->skills.end()) {
                    issues.push_back({Severity::Error, path, "task skill '" + task_node.skill + "' not found in vehicle capabilities"});
                } else {
                    validate_params(task_node.params, skill_it->second, path, issues);
                }
            }

            break;
        }
            
        default:
            break;
    }



} 

std::vector<Issue> validate(const Plan & plan, const VehicleCapabilities *capabilities) {
    // Validate if the plan structure is well-posed and compatible with the given capabilities. A list of issues is returned if any.
    std::vector<Issue> issues;

    if (plan.plan_id.empty()) {
        issues.push_back({Severity::Warning, "plan_id", "plan_id must be non-empty"});
    }

    if (plan.schema_version != kSchemaVersion) {
        issues.push_back({Severity::Error, "schema_version", "unsupported schema_version: " + std::to_string(plan.schema_version) + ", expected: " + std::to_string(kSchemaVersion)});
    } 

    if (!plan.root) {
        issues.push_back({Severity::Error, "root", "plan has no root node"});
        return issues;
    }

    // Validate the plan tree recursively, starting from the root node.
    validate_node(*plan.root, "root", capabilities, issues);

    return issues;
}

bool has_errors(const std::vector<Issue> &issues) {
    for (const auto &issue : issues) {
        if (issue.severity == Severity::Error) {
            return true;
        }
    }
    return false;
} 

} // namespace asr_mission
