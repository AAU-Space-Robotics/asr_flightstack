#pragma once

// Pre-arm confirmation overlay -- gated behind main.cpp's require_checklist param.
// Drawn directly onto the shared foreground ImDrawList (like ArmButton/ModeToggle in
// widgets.cpp), NOT via ImGui::BeginPopupModal -- this app's whole custom UI is painted
// onto ImGui::GetForegroundDrawList(), which ImGui always composites *after* every real
// ImGui window (popups included -- see ImGui::Render()), so a real popup here would
// always end up invisible underneath the map/panels. main.cpp must call Draw() last in
// the frame (after ImGui::End(), before ImGui::Render()) so nothing painted earlier that
// frame can cover it.

#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

#include "state_manager.h"

// Live telemetry the checklist reads each frame, plus the configured thresholds
// (min_battery_percent/min_satellites -- see checklist.yaml) it checks them against.
struct PreArmSnapshot {
    RTK_STATUS rtk_status = RTK_STATUS::INACTIVE;
    double battery_percent = 0.0;       // BATTERY_MAIN charge_remaining * 100 -- caller converts from PX4's 0-1 fraction
    float min_battery_percent = 40.0f;
    bool estop_engaged = false;
    int satellites_used = 0;
    int min_satellites = 6;
};

// Conditions severe enough that arming isn't allowed at all -- no override checkbox,
// unlike the confirm-to-proceed rows (RTK/battery). Empty means nothing is blocking.
// Shared between the always-visible issues badge next to Arm and the checklist dropdown
// itself, so both read the same list for the same frame.
std::vector<std::string> PreArmHardBlockIssues(const PreArmSnapshot& snapshot);

// Small red count badge, e.g. pinned to the corner of the Arm button. No-op if issue_count <= 0.
void DrawIssuesBadge(ImDrawList* draw_list, ImVec2 center, float scale, int issue_count);

class PreArmChecklist {
public:
    // Resets all confirmations -- the checklist must be re-confirmed on every arm
    // attempt. `on_confirm` fires once, the frame the operator clicks Confirm & Arm.
    void Open(std::function<void()> on_confirm);

    // No-op if not currently open. Call once per frame, last (see note above).
    // `anchor` is the top-center screen point the dropdown hangs from (already
    // scaled, same convention as ArmButton's `center` param) -- typically just
    // below the Arm button. `hard_issues` should be PreArmHardBlockIssues(snapshot),
    // computed once by the caller and shared with the issues badge.
    void Draw(ImDrawList* draw_list, ImVec2 anchor, float scale, bool theme,
              const PreArmSnapshot& snapshot, const std::vector<std::string>& hard_issues);

    bool IsOpen() const { return open_; }

private:
    bool open_ = false;
    std::function<void()> on_confirm_;

    bool inspection_confirmed_ = false;
    bool area_clear_confirmed_ = false;
    bool battery_confirmed_ = false;
    bool rtk_confirmed_ = false;

    void Close();
};
