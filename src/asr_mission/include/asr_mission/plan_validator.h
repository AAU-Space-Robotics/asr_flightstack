// Static plan validation.
//
// Used twice with the same code: on the GCS before upload (UX) and on the
// vehicle after upload (authority -- the vehicle never trusts GCS-side
// checks). Structural checks always run; skill/param/condition checks only
// run when `capabilities` is non-null.
#pragma once

#include <string>
#include <vector>

#include "asr_mission/capabilities.h"
#include "asr_mission/plan.h"

namespace asr_mission {

// Hard cap so no plan can loop unbounded; a supervisor above the executor
// still owns battery/geofence/link failsafes regardless of what a plan says.
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
