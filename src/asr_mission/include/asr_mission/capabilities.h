// Vehicle skill capabilities.
//
// Declares what one vehicle can do: its skills (with parameter specs/limits)
// and the condition predicates its executor can evaluate. The source of
// truth is <vehicle_pkg>/config/skills.yaml, installed to the package share
// directory -- the GCS and the onboard executor load the same file, and the
// connect handshake compares hash() to detect checkout skew between the two.
#pragma once

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace asr_mission {

// "float", "int", "bool", "string", "point" ([x,y,z]), "polygon" ([[x,y],...])
constexpr const char *kParamTypes[] = {"float", "int", "bool", "string", "point", "polygon"};

struct ParamSpec {
    std::string type;
    bool required = true;
    std::optional<double> min;
    std::optional<double> max;
};

struct SkillSpec {
    std::string name;
    std::map<std::string, ParamSpec> params;
};

class VehicleCapabilities {
public:
    std::string vehicle;
    std::map<std::string, SkillSpec> skills;
    std::vector<std::string> conditions;

    // 16 hex chars (8 bytes) over a canonicalized encoding, for the handshake.
    std::string hash() const;

    static VehicleCapabilities from_yaml(const std::string &path);
};

// Finds every installed skills.yaml, keyed by vehicle name, by scanning
// <prefix>/share/*/config/skills.yaml for each prefix in AMENT_PREFIX_PATH.
std::map<std::string, VehicleCapabilities> discover_capabilities();

} // namespace asr_mission
