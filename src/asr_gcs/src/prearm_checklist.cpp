#include "prearm_checklist.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "info_panels.h"

namespace {

const char* RtkStatusLabel(RTK_STATUS status) {
    switch (status) {
        case RTK_STATUS::INACTIVE:  return "INACTIVE";
        case RTK_STATUS::SURVEY_IN: return "SURVEY-IN";
        case RTK_STATUS::STREAMING: return "STREAMING";
    }
    return "UNKNOWN";
}

constexpr float kIssueLineGap = 6.0f;      // between hard-block issue lines, unscaled
constexpr float kIssuesSectionGap = 12.0f; // above the issues block, on top of the last row's kRowGap

constexpr float kRowButtonW = 110.0f;   // unscaled, per-row Confirm button
constexpr float kRowButtonH = 28.0f;
constexpr float kRowButtonGap = 16.0f;  // between label text and its Confirm button

bool DrawChecklistButton(ImDrawList* draw_list, ImVec2 center, ImVec2 half_size, const UiScale& scale,
                          const char* label, bool enabled, ImVec4 base) {
    ImVec2 bb_min(center.x - half_size.x, center.y - half_size.y);
    ImVec2 bb_max(center.x + half_size.x, center.y + half_size.y);

    bool hovered = enabled && ImGui::IsMouseHoveringRect(bb_min, bb_max, false);
    bool active = hovered && ImGui::IsMouseDown(0);
    bool released = hovered && ImGui::IsMouseReleased(0);

    ImVec4 color = base;
    if (!enabled) {
        color.w = 0.35f;
    } else if (active) {
        color = ImVec4(base.x * 0.7f, base.y * 0.7f, base.z * 0.7f, base.w);
    } else if (hovered) {
        color = ImVec4(base.x + 0.15f, base.y + 0.15f, base.z + 0.15f, base.w);
    }

    draw_list->AddRectFilled(bb_min, bb_max, ImGui::ColorConvertFloat4ToU32(color), 8.0f * scale.uniform());

    ImVec2 text_size = ImGui::CalcTextSize(label);
    draw_list->AddText(ImVec2(center.x - text_size.x / 2, center.y - text_size.y / 2),
                        IM_COL32(255, 255, 255, 255), label);

    return released;
}

bool DrawCheckRow(ImDrawList* draw_list, ImVec2 row_min, ImVec2 row_max, const UiScale& scale, bool theme,
                   bool checked, bool interactive, const char* label, float wrap_width) {
    const float row_h = row_max.y - row_min.y;

    ImU32 text_color = interactive ? Color::white_black(theme) : Color::dwhite_lblack(theme);
    ImVec2 text_size = ImGui::CalcTextSize(label, nullptr, false, wrap_width);
    ImVec2 text_pos(row_min.x, row_min.y + (row_h - text_size.y) * 0.5f);
    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), text_pos, text_color, label, nullptr, wrap_width);

    const float button_w = kRowButtonW * scale.x;
    const float button_h = kRowButtonH * scale.y;
    ImVec2 button_center(row_max.x - button_w * 0.5f, row_min.y + row_h * 0.5f);
    ImVec2 button_half(button_w * 0.5f, button_h * 0.5f);

    const char* button_label = !interactive ? "Locked" : (checked ? "Confirmed" : "Confirm");
    ImVec4 base = !interactive ? ImVec4(0.35f, 0.35f, 0.35f, 1.0f)
                : checked      ? ImVec4(0.16f, 0.55f, 0.24f, 1.0f)
                               : ImVec4(0.902f, 0.494f, 0.133f, 1.0f);

    return DrawChecklistButton(draw_list, button_center, button_half, scale, button_label, interactive, base);
}

// Row height needed for `label` wrapped to `wrap_width`, at least tall enough for its Confirm button.
float CheckRowHeight(const char* label, const UiScale& scale, float wrap_width) {
    ImVec2 text_size = ImGui::CalcTextSize(label, nullptr, false, wrap_width);
    return std::max(kRowButtonH * scale.y, text_size.y);
}

constexpr float kPanelW = 460.0f;
constexpr float kPadding = 24.0f;
constexpr float kTitleH = 26.0f;
constexpr float kGapAfterTitle = 20.0f;
constexpr float kRowGap = 16.0f;
constexpr float kGapBeforeButtons = 16.0f;   // on top of the trailing kRowGap after the last row
constexpr float kButtonH = 38.0f;
constexpr float kButtonGap = 12.0f;
constexpr int kRowCount = 4;
constexpr int kBottomButtonCount = 3;   // Cancel | Reset | Confirm & Arm

}  // namespace

std::vector<std::string> PreArmHardBlockIssues(const PreArmSnapshot& snapshot) {
    std::vector<std::string> issues;
    if (snapshot.estop_engaged) {
        issues.push_back("E-STOP is engaged");
    }
    if (snapshot.satellites_used < snapshot.min_satellites) {
        issues.push_back("GPS satellites: " + std::to_string(snapshot.satellites_used) +
                          " (need " + std::to_string(snapshot.min_satellites) + "+)");
    }
    return issues;
}

void DrawIssuesBadge(ImDrawList* draw_list, ImVec2 center, const UiScale& scale, int issue_count) {
    if (issue_count <= 0) { return; }
    const float radius = 12.0f * scale.uniform();
    draw_list->AddCircleFilled(center, radius, IM_COL32(220, 60, 60, 255));
    draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 0, 1.5f * scale.uniform());
    char label[16];
    std::snprintf(label, sizeof(label), "%d", issue_count);
    ImVec2 text_size = ImGui::CalcTextSize(label);
    draw_list->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
                        IM_COL32(255, 255, 255, 255), label);
}

void PreArmChecklist::Open(std::function<void()> on_confirm) {
    on_confirm_ = std::move(on_confirm);
    inspection_confirmed_ = false;
    area_clear_confirmed_ = false;
    battery_confirmed_ = false;
    rtk_confirmed_ = false;
    open_ = true;
}

void PreArmChecklist::Close() {
    open_ = false;
}

void PreArmChecklist::Draw(ImDrawList* draw_list, ImVec2 anchor, const UiScale& scale, bool theme,
                            const PreArmSnapshot& snapshot, const std::vector<std::string>& hard_issues) {
    if (!open_) { return; }

    const bool rtk_streaming = (snapshot.rtk_status == RTK_STATUS::STREAMING);
    if (rtk_streaming) {
        rtk_confirmed_ = true;
    }
    const bool battery_ok = snapshot.battery_percent >= snapshot.min_battery_percent;
    if (battery_ok) {
        battery_confirmed_ = true;
    }

    char rtk_label[96];
    if (rtk_streaming) {
        std::snprintf(rtk_label, sizeof(rtk_label), "RTK: %s -- locked", RtkStatusLabel(snapshot.rtk_status));
    } else {
        std::snprintf(rtk_label, sizeof(rtk_label), "RTK: %s -- confirm arm without RTK lock",
                       RtkStatusLabel(snapshot.rtk_status));
    }
    char battery_label[96];
    if (battery_ok) {
        std::snprintf(battery_label, sizeof(battery_label), "Motor battery: %.0f%% -- locked", snapshot.battery_percent);
    } else {
        std::snprintf(battery_label, sizeof(battery_label), "Motor battery: %.0f%% -- confirm arm below %.0f%%",
                       snapshot.battery_percent, snapshot.min_battery_percent);
    }

    struct Row { const char* label; bool* value; bool interactive; };
    Row rows[kRowCount] = {
        {"Motors/ESCs OK, propellers installed and undamaged, visual inspection done",
         &inspection_confirmed_, true},
        {"Area/takeoff zone clear of people and obstacles", &area_clear_confirmed_, true},
        {battery_label, &battery_confirmed_, !battery_ok},
        {rtk_label, &rtk_confirmed_, !rtk_streaming},
    };

    const float wrap_width = kPanelW * scale.x - 2 * kPadding * scale.x
                            - kRowButtonW * scale.x - kRowButtonGap * scale.x;

    float row_heights[kRowCount];
    float rows_total = 0.0f;
    for (int i = 0; i < kRowCount; ++i) {
        row_heights[i] = CheckRowHeight(rows[i].label, scale, wrap_width);
        rows_total += row_heights[i];
        if (i + 1 < kRowCount) { rows_total += kRowGap * scale.y; }
    }

    // Hard-block issues (no override) get their own lines under the confirm rows.
    std::vector<float> issue_heights(hard_issues.size());
    float issues_total = 0.0f;
    for (size_t i = 0; i < hard_issues.size(); ++i) {
        std::string text = "Cannot arm: " + hard_issues[i];
        issue_heights[i] = ImGui::CalcTextSize(text.c_str(), nullptr, false, wrap_width).y;
        issues_total += issue_heights[i];
        if (i + 1 < hard_issues.size()) { issues_total += kIssueLineGap * scale.y; }
    }
    if (!hard_issues.empty()) {
        issues_total += kIssuesSectionGap * scale.y;
    }

    const float panel_h = 2 * kPadding * scale.y + kTitleH * scale.y + kGapAfterTitle * scale.y
                         + rows_total + issues_total + kGapBeforeButtons * scale.y
                         + kButtonH * scale.y;

    ImVec2 panel_min(anchor.x - kPanelW * 0.5f * scale.x, anchor.y);
    ImVec2 panel_size(kPanelW * scale.x, panel_h);

    // Soft drop shadow so it reads as floating above the map/panels behind it.
    draw_list->AddRectFilled(ImVec2(panel_min.x + 3.0f * scale.x, panel_min.y + 4.0f * scale.y),
                              ImVec2(panel_min.x + panel_size.x + 3.0f * scale.x, panel_min.y + panel_size.y + 4.0f * scale.y),
                              IM_COL32(0, 0, 0, 90), 16.0f * scale.uniform());
    DrawPanelBackground(draw_list, panel_min, panel_size,
                         ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)),
                         Color::panelBorder(theme), 16.0f * scale.uniform(), 2.0f * scale.uniform());

    float cursor_y = panel_min.y + kPadding * scale.y;
    const float content_x = panel_min.x + kPadding * scale.x;
    const float content_right = panel_min.x + panel_size.x - kPadding * scale.x;

    ImGui::PushFont(winInit.getFont(181));  // bold 18
    draw_list->AddText(ImVec2(content_x, cursor_y), Color::white_black(theme), "PRE-ARM CHECKLIST");
    ImGui::PopFont();
    cursor_y += kTitleH * scale.y + kGapAfterTitle * scale.y;

    for (int i = 0; i < kRowCount; ++i) {
        ImVec2 row_min(content_x, cursor_y);
        ImVec2 row_max(content_right, cursor_y + row_heights[i]);
        if (DrawCheckRow(draw_list, row_min, row_max, scale, theme, *rows[i].value, rows[i].interactive,
                          rows[i].label, wrap_width)) {
            *rows[i].value = !*rows[i].value;
        }
        cursor_y += row_heights[i];
        if (i + 1 < kRowCount) { cursor_y += kRowGap * scale.y; }
    }

    if (!hard_issues.empty()) {
        cursor_y += kIssuesSectionGap * scale.y;
        for (size_t i = 0; i < hard_issues.size(); ++i) {
            std::string text = "Cannot arm: " + hard_issues[i];
            draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(content_x, cursor_y),
                                IM_COL32(255, 92, 92, 255), text.c_str(), nullptr, wrap_width);
            cursor_y += issue_heights[i];
            if (i + 1 < hard_issues.size()) { cursor_y += kIssueLineGap * scale.y; }
        }
    }

    cursor_y += kGapBeforeButtons * scale.y;

    const bool can_arm = inspection_confirmed_ && area_clear_confirmed_ && battery_confirmed_
                        && rtk_confirmed_ && hard_issues.empty();
    const float total_gap = kButtonGap * scale.x * (kBottomButtonCount - 1);
    const float bottom_button_w = (panel_size.x - 2 * kPadding * scale.x - total_gap) / kBottomButtonCount;
    ImVec2 button_half(bottom_button_w * 0.5f, kButtonH * 0.5f * scale.y);
    const float button_y = cursor_y + kButtonH * 0.5f * scale.y;

    float button_x = content_x + bottom_button_w * 0.5f;
    if (DrawChecklistButton(draw_list, ImVec2(button_x, button_y), button_half, scale, "Cancel", true,
                             ImVec4(0.35f, 0.35f, 0.35f, 1.0f))) {
        Close();
        return;
    }

    button_x += bottom_button_w + kButtonGap * scale.x;
    if (DrawChecklistButton(draw_list, ImVec2(button_x, button_y), button_half, scale, "Reset", true,
                             ImVec4(0.25f, 0.35f, 0.55f, 1.0f))) {
        inspection_confirmed_ = false;
        area_clear_confirmed_ = false;
        battery_confirmed_ = false;
        rtk_confirmed_ = false;
        return;
    }

    button_x += bottom_button_w + kButtonGap * scale.x;
    if (DrawChecklistButton(draw_list, ImVec2(button_x, button_y), button_half, scale, "Confirm & Arm", can_arm,
                             ImVec4(0.902f, 0.494f, 0.133f, 1.0f))) {
        auto on_confirm = std::move(on_confirm_);
        Close();
        on_confirm();
    }
}
