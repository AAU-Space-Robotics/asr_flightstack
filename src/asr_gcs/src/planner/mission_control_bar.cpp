#include "planner/mission_control_bar.h"

#include <algorithm>
#include <filesystem>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "info_panels.h"
#include "planner/planner.h"

using namespace asr_mission;

namespace {

std::string PlansDirectory() {
    return ament_index_cpp::get_package_share_directory("asr_mission") + "/plans";
}

void PushThemedButtonStyle(bool theme, bool active) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::PushStyleColor(ImGuiCol_Button, active ? IM_COL32(255, 130, 30, 255) : Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 130, 30, 255));
}

void PopThemedButtonStyle() {
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
}

// Hand-drawn, not a Unicode glyph -- matches planner_panel.cpp's own DrawCheckmark/DrawCross.
void DrawCheckmark(ImDrawList *draw_list, ImVec2 center, float size, ImU32 color, float thickness) {
    const ImVec2 p1(center.x - size * 0.5f, center.y);
    const ImVec2 p2(center.x - size * 0.1f, center.y + size * 0.4f);
    const ImVec2 p3(center.x + size * 0.5f, center.y - size * 0.4f);
    draw_list->AddLine(p1, p2, color, thickness);
    draw_list->AddLine(p2, p3, color, thickness);
}

void DrawCross(ImDrawList *draw_list, ImVec2 center, float size, ImU32 color, float thickness) {
    draw_list->AddLine(ImVec2(center.x - size * 0.5f, center.y - size * 0.5f),
                       ImVec2(center.x + size * 0.5f, center.y + size * 0.5f), color, thickness);
    draw_list->AddLine(ImVec2(center.x - size * 0.5f, center.y + size * 0.5f),
                       ImVec2(center.x + size * 0.5f, center.y - size * 0.5f), color, thickness);
}

} // namespace

float MissionControlBar::PanelTopY(float scale, float map_y, float map_h) const
{
    const float bar_h = ImGui::GetFontSize() + 20.0f * scale;
    const float panel_h = bar_h + 16.0f * scale;
    return map_y + map_h - panel_h - 20.0f * scale;
}

float MissionControlBar::Draw(Planner &planner, float scale, bool theme,
                               float map_x, float map_y, float map_w, float map_h,
                               const std::function<void(std::function<void()>)> &guard_execute)
{
    const std::vector<Issue> issues = planner.local_issues();
    size_t error_count = 0, warning_count = 0;
    for (const auto &issue : issues) {
        if (issue.severity == Severity::Error) { ++error_count; } else { ++warning_count; }
    }
    const bool has_tasks = planner.has_tasks();

    const UploadStatus upload_status = planner.upload_status();
    const std::vector<Issue> upload_issues = planner.upload_issues();
    const bool upload_clean = upload_status == UploadStatus::Validated &&
        std::none_of(upload_issues.begin(), upload_issues.end(),
                     [](const Issue &i) { return i.severity == Severity::Error; });
    const MissionStatus mission_status = planner.mission_status();
    const bool can_execute = upload_clean && mission_status != MissionStatus::Running;
    const bool can_abort = mission_status == MissionStatus::Running;

    // --- Sizing: total width from the actual label sizes, then center on the map --
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale, 10.0f * scale));
    const float frame_pad_x = ImGui::GetStyle().FramePadding.x;
    const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
    static const char *kLabels[] = {"Load", "Clear", "Upload", "Execute", "Abort"};
    float content_w = 0.0f;
    for (const char *label : kLabels) {
        content_w += ImGui::CalcTextSize(label).x + frame_pad_x * 2.0f;
    }
    const float bar_h = ImGui::GetFontSize() + 20.0f * scale;
    const float separator_w = 8.0f * scale;
    content_w += bar_h;          // result icon, next to Upload -- icon_span == bar_h (same GetFrameHeight())
    content_w += separator_w;    // divider between the plan-staging group and Execute/Abort
    content_w += item_spacing * 6.0f;  // 6 gaps: Load|Clear|Upload|icon|separator|Execute|Abort
    const float panel_w = content_w + 16.0f * scale;
    const float panel_h = bar_h + 16.0f * scale;
    ImGui::PopStyleVar();

    const float panel_x = map_x + (map_w - panel_w) * 0.5f;
    const float panel_y = map_y + map_h - panel_h - 20.0f * scale;

    BeginFixedPanel("MissionControlBar", ImVec2(panel_x, panel_y), ImVec2(panel_w, panel_h),
                     scale, theme, 0, ImVec2(8, 8));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale, 10.0f * scale));

    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Load")) {
        save_load_dialog_.Open(SaveLoadDialog::Mode::Load, PlansDirectory(), ".json",
            [&planner](const std::string &path) { planner.load(path); });
    }
    PopThemedButtonStyle();
    ImGui::SameLine();

    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Clear")) {
        ImGui::OpenPopup("ConfirmClearPopupBar");
    }
    PopThemedButtonStyle();
    ImGui::SameLine();

    // Upload: same red/yellow/normal/greyed treatment as the Planner tab's own Upload button.
    ImGui::BeginDisabled(!has_tasks);
    if (!has_tasks) {
        PushThemedButtonStyle(theme, false);
    } else if (error_count > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
    } else if (warning_count > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(150, 110, 20, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(178, 130, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(198, 145, 30, 255));
    } else {
        PushThemedButtonStyle(theme, false);
    }
    if (ImGui::Button("Upload")) {
        planner.upload();
    }
    if (!has_tasks) {
        PopThemedButtonStyle();
    } else if (error_count > 0 || warning_count > 0) {
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
    } else {
        PopThemedButtonStyle();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();

    // Compact result indicator -- same hand-drawn check/cross/ring as the Planner tab's Upload button.
    {
        const float icon_span = ImGui::GetFrameHeight();
        ImGui::Dummy(ImVec2(icon_span, icon_span));
        const ImVec2 icon_min = ImGui::GetItemRectMin();
        const ImVec2 icon_max = ImGui::GetItemRectMax();
        const ImVec2 icon_center((icon_min.x + icon_max.x) * 0.5f, (icon_min.y + icon_max.y) * 0.5f);
        const float icon_size = icon_span * 0.55f;
        ImDrawList *bar_draw_list = ImGui::GetWindowDrawList();
        // Boxed like the buttons it sits next to, or it reads as oddly spaced.
        bar_draw_list->AddRectFilled(icon_min, icon_max, Color::panelBorder(theme), 12.0f);

        std::string tooltip_header;
        std::vector<Issue> icon_issues;
        switch (upload_status) {
            case UploadStatus::Idle:
                // Dim dash rather than an empty box -- reads as "nothing yet", not a glitch.
                bar_draw_list->AddLine(ImVec2(icon_center.x - icon_size * 0.3f, icon_center.y),
                                       ImVec2(icon_center.x + icon_size * 0.3f, icon_center.y),
                                       Color::dwhite_lblack(theme), 2.5f * scale);
                tooltip_header = "Not uploaded yet";
                break;
            case UploadStatus::Uploading:
                bar_draw_list->AddCircle(icon_center, icon_size * 0.5f, IM_COL32(245, 200, 76, 255), 0, 2.0f * scale);
                tooltip_header = "Uploading -- waiting for vehicle...";
                break;
            case UploadStatus::TimedOut:
                DrawCross(bar_draw_list, icon_center, icon_size, IM_COL32(255, 92, 92, 255), 2.5f * scale);
                tooltip_header = "No response from vehicle -- click Upload to retry";
                break;
            case UploadStatus::Validated: {
                icon_issues = upload_issues;
                size_t up_errors = 0, up_warnings = 0;
                for (const auto &issue : icon_issues) {
                    if (issue.severity == Severity::Error) { ++up_errors; } else { ++up_warnings; }
                }
                if (up_errors > 0) {
                    DrawCross(bar_draw_list, icon_center, icon_size, IM_COL32(255, 92, 92, 255), 2.5f * scale);
                    tooltip_header = std::to_string(up_errors) + (up_errors == 1 ? " error" : " errors") + " on vehicle";
                } else if (up_warnings > 0) {
                    DrawCheckmark(bar_draw_list, icon_center, icon_size, IM_COL32(245, 200, 76, 255), 2.5f * scale);
                    tooltip_header = std::to_string(up_warnings) + (up_warnings == 1 ? " warning" : " warnings") + " on vehicle";
                } else {
                    DrawCheckmark(bar_draw_list, icon_center, icon_size, IM_COL32(100, 220, 100, 255), 2.5f * scale);
                    tooltip_header = "Validated on vehicle -- no issues";
                }
                break;
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tooltip_header.c_str());
            for (const auto &issue : icon_issues) {
                const ImVec4 color = issue.severity == Severity::Error
                    ? ImVec4(1.0f, 0.36f, 0.36f, 1.0f) : ImVec4(0.96f, 0.78f, 0.30f, 1.0f);
                ImGui::TextColored(color, "%s: %s", issue.path.c_str(), issue.message.c_str());
            }
            ImGui::EndTooltip();
        }
    }
    ImGui::SameLine();

    // Divider between plan-staging (Load/Clear/Upload) and flight control (Execute/Abort).
    {
        const float sep_h = ImGui::GetFrameHeight();
        ImGui::Dummy(ImVec2(separator_w, sep_h));
        const ImVec2 sep_min = ImGui::GetItemRectMin();
        const ImVec2 sep_max = ImGui::GetItemRectMax();
        const float sep_x = (sep_min.x + sep_max.x) * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(sep_x, sep_min.y), ImVec2(sep_x, sep_max.y),
                                            Color::panelBorder(theme), 1.5f * scale);
    }
    ImGui::SameLine();

    ImGui::BeginDisabled(!can_execute);
    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Execute")) {
        guard_execute([&planner]() { planner.start(); });
    }
    PopThemedButtonStyle();
    ImGui::EndDisabled();
    ImGui::SameLine();

    // Abort: only ever usable while a mission is actually running.
    ImGui::BeginDisabled(!can_abort);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
    if (ImGui::Button("Abort")) {
        planner.abort();
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    ImGui::EndDisabled();

    ImGui::PopStyleVar();  // FramePadding

    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("ConfirmClearPopupBar", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("Clear the entire flight plan?");
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
        if (ImGui::Button("Yes, clear it")) {
            planner.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::SameLine();
        PushThemedButtonStyle(theme, false);
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        PopThemedButtonStyle();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);

    // Must run before EndFixedPanel() -- popup IDs are salted by the current window's ID stack.
    save_load_dialog_.Draw(scale, theme);

    EndFixedPanel();
    return panel_y;
}
