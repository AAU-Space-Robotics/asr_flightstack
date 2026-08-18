#include "asr_mission/capabilities.h"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <openssl/evp.h>
#include <yaml-cpp/yaml.h>

namespace asr_mission {

std::string VehicleCapabilities::hash() const {
    // Build a deterministic string over vehicle, sorted skills+params, sorted conditions.
    std::ostringstream ss;
    ss << "vehicle=" << vehicle << "\n";
    for (const auto &[skill_name, spec] : skills) {  // map iterates in key order
        ss << "skill=" << skill_name << "\n";
        for (const auto &[param_name, ps] : spec.params) {
            ss << "param=" << param_name << " type=" << ps.type
               << " required=" << ps.required;
            if (ps.min) { ss << " min=" << *ps.min; }
            if (ps.max) { ss << " max=" << *ps.max; }
            ss << "\n";
        }
    }
    for (const auto &cond : conditions) {
        ss << "cond=" << cond << "\n";
    }

    std::string canonical = ss.str();

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, canonical.data(), canonical.size());
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Truncate to 16 hex chars (8 bytes)
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < 8; ++i) {
        hex << std::setw(2) << static_cast<int>(digest[i]);
    }
    return hex.str();
}

VehicleCapabilities VehicleCapabilities::from_yaml(const std::string &path) {
    YAML::Node root = YAML::LoadFile(path);

    VehicleCapabilities caps;
    caps.vehicle = root["vehicle"].as<std::string>();

    // Parse skills
    YAML::Node skills_node = root["skills"];
    for (const auto &skill_entry : skills_node) {
        std::string skill_name = skill_entry.first.as<std::string>();
        SkillSpec spec;
        spec.name = skill_name;

        YAML::Node params_node = skill_entry.second["params"];
        for (const auto &param_entry : params_node) {
            std::string param_name = param_entry.first.as<std::string>();
            YAML::Node p = param_entry.second;

            ParamSpec ps;
            ps.type = p["type"].as<std::string>();
            ps.required = p["required"] ? p["required"].as<bool>() : true;
            if (p["min"]) { ps.min = p["min"].as<double>(); }
            if (p["max"]) { ps.max = p["max"].as<double>(); }

            spec.params[param_name] = ps;
        }

        caps.skills[skill_name] = spec;
    }

    // Parse conditions
    YAML::Node conditions_node = root["conditions"];
    for (const auto &cond : conditions_node) {
        caps.conditions.push_back(cond.as<std::string>());
    }

    return caps;
}

std::map<std::string, VehicleCapabilities> discover_capabilities() {
    std::map<std::string, VehicleCapabilities> result;

    const char *prefix_path_env = std::getenv("AMENT_PREFIX_PATH");
    if (!prefix_path_env) {
        return result;
    }

    // AMENT_PREFIX_PATH is colon-separated
    std::istringstream stream(prefix_path_env);
    std::string prefix;
    while (std::getline(stream, prefix, ':')) {
        std::filesystem::path share_dir = std::filesystem::path(prefix) / "share";
        if (!std::filesystem::exists(share_dir)) { continue; }

        for (const auto &pkg_entry : std::filesystem::directory_iterator(share_dir)) {
            std::filesystem::path skills_yaml = pkg_entry.path() / "config" / "uav" / "skills.yaml";
            if (!std::filesystem::exists(skills_yaml)) { continue; }

            VehicleCapabilities caps = VehicleCapabilities::from_yaml(skills_yaml.string());
            // First match per vehicle name wins (mirrors ament overlay ordering)
            if (result.count(caps.vehicle) == 0) {
                result[caps.vehicle] = std::move(caps);
            }
        }
    }

    return result;
}

} // namespace asr_mission
