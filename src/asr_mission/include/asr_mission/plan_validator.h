// Static plan validation, used identically on the GCS (UX) and the vehicle (authority).
#pragma once

#include <string>
#include <vector>

#include "asr_mission/capabilities.h"
#include "asr_mission/plan.h"

namespace asr_mission {

// Hard cap so no plan can loop unbounded.
constexpr int kMaxRetryAttempts = 25;
constexpr int kMaxRepeatCount = 25;

enum class Severity { Warning, Error };

struct Issue {
    Severity severity;
    std::string path;
    std::string message;

    std::string to_string() const;
};

std::vector<Issue> validate(const Plan &plan, const VehicleCapabilities *capabilities = nullptr);

bool has_errors(const std::vector<Issue> &issues);

} // namespace asr_mission
