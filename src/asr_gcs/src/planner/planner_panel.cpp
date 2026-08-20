#include "planner/planner_panel.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <regex>
#include <string>
#include <vector>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "info_panels.h"

using namespace asr_mission;

namespace {

// Save/Load only ever deal in a name within here, never an arbitrary path.
std::string PlansDirectory() {
    return ament_index_cpp::get_package_share_directory("asr_mission") + "/plans";
}

// `active` uses the sidebar's orange highlight for "currently selected."
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

// Display-only -- Planner still gets the real lowercase name.
std::string ToUpper(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

// Generic "key: value" join, works for any skill's param set.
std::string TaskSubtitle(const TaskNode &task) {
    if (task.params.empty()) {
        return "no params";
    }
    std::string subtitle;
    bool first = true;
    for (auto it = task.params.begin(); it != task.params.end(); ++it) {
        if (!first) { subtitle += "   "; }
        first = false;
        subtitle += it.key() + ": " + it.value().dump();
    }
    return subtitle;
}

std::string FormatNumber(double value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", value);
    return buf;
}

// Ellipsizes to fit max_width -- plan names can otherwise run on under the fixed-position status pills.
std::string TruncateToWidth(const std::string &text, float max_width) {
    if (ImGui::CalcTextSize(text.c_str()).x <= max_width) {
        return text;
    }
    std::string truncated = text;
    while (!truncated.empty() && ImGui::CalcTextSize((truncated + "...").c_str()).x > max_width) {
        truncated.pop_back();
    }
    return truncated.empty() ? "..." : truncated + "...";
}

// Condition has no declared type (unlike a skill's ParamSpec), so this is a stopgap until the manifest schema carries one.
bool ConditionValueIsInt(const std::string &cond) {
    return cond == "probes_found";
}


std::string ReadableIssuePath(const std::string &path) {
    static const std::regex kPattern(
        R"(^root\.children\[(\d+)\](?:\.child\.children\[(\d+)\])?(?:\.conditions_any\[(\d+)\]|\.params\.(.+))?$)");
    std::smatch m;
    if (!std::regex_match(path, m, kPattern)) {
        return path;  // plan-level (plan_id/schema_version) or unrecognized shape -- show as-is
    }

    std::string result = "Task " + std::to_string(std::stoi(m[1].str()) + 1);
    if (m[2].matched) {
        result += ", Subtask " + std::to_string(std::stoi(m[2].str()) + 1);
    }
    if (m[3].matched) {
        result += ", Condition " + std::to_string(std::stoi(m[3].str()) + 1);
    } else if (m[4].matched) {
        result += " (" + m[4].str() + ")";
    }
    return result;
}

// "3 tasks  ·  <detail>" -- shared prefix for any wrapper kind.
std::string WrapperSubtitle(size_t task_count, const std::string &detail) {
    return std::to_string(task_count) + (task_count == 1 ? " task" : " tasks") + "  \xC2\xB7  " + detail;
}

// "battery_low <= 20  or  time_elapsed >= 60"
std::string ConditionsSummary(const RunUntilNode &group) {
    if (group.conditions_any.empty()) {
        return "no conditions";
    }
    std::string summary;
    bool first = true;
    for (const auto &c : group.conditions_any) {
        if (!first) { summary += "  or  "; }
        first = false;
        summary += c.cond;
        if (c.op && c.value) {
            summary += " " + *c.op + " " + FormatNumber(*c.value);
        }
    }
    return summary;
}

size_t WrapperChildTaskCount(const PlanNode *child) {
    if (child && child->kind() == NodeKind::Sequence) {
        return static_cast<const SequenceNode &>(*child).children.size();
    }
    return 0;
}

// Hand-drawn rather than a Unicode glyph -- the loaded font atlas doesn't cover check/cross codepoints.
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

PlannerPanel::PlannerPanel(Planner &planner)
    : planner_(planner)
{
}

void PlannerPanel::Draw(const UiScale& scale, bool theme, float window_height)
{
    DrawVehicleAndPalette(scale, theme);
    DrawTaskList(scale, theme, window_height);
}

std::pair<int, int> PlannerPanel::highlighted_task() const
{
    if (expanded_task_index_ < 0) { return {-1, -1}; }

    const PlanNode *root = planner_.plan().root.get();
    const SequenceNode *sequence = (root && root->kind() == NodeKind::Sequence)
        ? static_cast<const SequenceNode *>(root) : nullptr;
    if (!sequence || static_cast<size_t>(expanded_task_index_) >= sequence->children.size()) {
        return {-1, -1};
    }

    const NodeKind kind = sequence->children[expanded_task_index_]->kind();
    const bool is_group = kind == NodeKind::RunUntil || kind == NodeKind::Repeat || kind == NodeKind::Retry;
    return {expanded_task_index_, is_group ? expanded_group_task_index_ : -1};
}

void PlannerPanel::SelectTask(size_t top_level_index, int nested_index)
{
    expanded_task_index_ = static_cast<int>(top_level_index);
    expanded_group_task_index_ = nested_index;
    scroll_to_expanded_ = true;
}

void PlannerPanel::DrawVehicleAndPalette(const UiScale& scale, bool theme)
{
    BeginFixedPanel("VehicleAndPalettePanel", ImVec2(70 * scale.x, 70 * scale.y), ImVec2(450 * scale.x, 140 * scale.y),
                     scale, theme, 0, ImVec2(8, 8));

    // --- Vehicle dropdown ------------------------------------------------
    const VehicleCapabilities *selected = planner_.selected_capabilities();
    const std::string preview_text = selected ? ToUpper(selected->vehicle) : std::string("SELECT VEHICLE...");

    // BeginCombo's preview area (FrameBg*) and arrow box (Button*) are separate slots.
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Button, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 130, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(255, 130, 30, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f * scale.x, 12.0f * scale.y));
    ImGui::PushFont(winInit.getFont(24));  // bold 24 -- this is the plan's primary identity control
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##VehicleSelect", preview_text.c_str())) {
        for (const auto &[name, capabilities] : planner_.available_vehicles()) {
            (void)capabilities;
            const bool is_selected = selected && selected->vehicle == name;
            if (ImGui::Selectable(ToUpper(name).c_str(), is_selected)) {
                planner_.select_vehicle(name);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(9);

    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::Separator();
    ImGui::PopStyleColor();

    // --- Palette: one button per skill the selected vehicle supports -----
    const VehicleCapabilities *capabilities = planner_.selected_capabilities();
    if (!capabilities) {
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color::dwhite_lblack(theme));
        ImGui::TextDisabled("Select a vehicle to see its available skills");
        ImGui::PopStyleColor();
    } else {
        std::vector<std::string> skill_names;
        for (const auto &[skill, spec] : capabilities->skills) {
            (void)spec;
            skill_names.push_back(skill);
        }

        const float window_right_edge = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f * scale.x, 8.0f * scale.y));
        bool any_hovered = false;
        for (size_t i = 0; i < skill_names.size(); ++i) {
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button(ToUpper(skill_names[i]).c_str())) {
                planner_.add_task(skill_names[i]);
            }
            PopThemedButtonStyle();

            if (ImGui::IsItemHovered()) {
                any_hovered = true;
                if (hovered_skill_ != skill_names[i]) {
                    hovered_skill_ = skill_names[i];
                    hover_start_time_ = ImGui::GetTime();
                }
                hovered_button_max_ = ImGui::GetItemRectMax();
                if (ImGui::GetTime() - hover_start_time_ >= kHoverTooltipDelay) {
                    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        IM_COL32(255, 130, 30, 255), 12.0f, 0, 2.5f * scale.uniform());
                }
            }

            if (i + 1 < skill_names.size()) {
                const float this_right = ImGui::GetItemRectMax().x;
                const float next_width = ImGui::CalcTextSize(ToUpper(skill_names[i + 1]).c_str()).x
                                        + ImGui::GetStyle().FramePadding.x * 2.0f;
                if (this_right + ImGui::GetStyle().ItemSpacing.x + next_width < window_right_edge) {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::PopStyleVar();
        if (!any_hovered) { hovered_skill_.clear(); }
    }

    EndFixedPanel();

    // Drawn after this panel closes -- BeginFixedPanel positions relative to the current window.
    if (capabilities && !hovered_skill_.empty() && ImGui::GetTime() - hover_start_time_ >= kHoverTooltipDelay) {
        DrawPaletteHoverToast(scale, theme, capabilities->skills.at(hovered_skill_).description);
    }
}

void PlannerPanel::DrawPaletteHoverToast(const UiScale& scale, bool theme, const std::string &description)
{
    if (description.empty()) { return; }

    ImFont *title_font = winInit.getFont(24);
    ImFont *body_font = ImGui::GetFont();
    const float title_font_size = title_font->LegacySize;
    const float body_font_size = ImGui::GetFontSize();

    const float pad_x = 16.0f * scale.x;
    const float pad_y = 12.0f * scale.y;
    const float toast_w = 340.0f * scale.x;
    const float wrap_w = toast_w - pad_x * 2.0f;
    const std::string title = ToUpper(hovered_skill_);

    const ImVec2 title_size = title_font->CalcTextSizeA(title_font_size, 9999.0f, wrap_w, title.c_str());
    const ImVec2 body_size = body_font->CalcTextSizeA(body_font_size, 9999.0f, wrap_w, description.c_str());
    const float toast_h = pad_y * 2.0f + title_size.y + 4.0f * scale.y + body_size.y;

    // Anchored to the hovered button's own bottom-right corner, extending down-right from it.
    const ImVec2 toast_min(hovered_button_max_.x + 6.0f * scale.x, hovered_button_max_.y + 6.0f * scale.y);
    const ImVec2 toast_max(toast_min.x + toast_w, toast_min.y + toast_h);

    // Foreground-drawn -- later-drawn sibling panels (height chart, planner map) would otherwise cover it.
    ImDrawList *draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddRectFilled(toast_min, toast_max, ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)), 12.0f * scale.uniform());
    draw_list->AddRect(toast_min, toast_max, Color::panelBorder(theme), 12.0f * scale.uniform(), 0, 1.5f * scale.uniform());

    const ImVec2 title_pos(toast_min.x + pad_x, toast_min.y + pad_y);
    draw_list->AddText(title_font, title_font_size, title_pos, Color::white_black(theme),
                       title.c_str(), nullptr, wrap_w);

    const ImVec2 body_pos(toast_min.x + pad_x, title_pos.y + title_size.y + 4.0f * scale.y);
    draw_list->AddText(body_font, body_font_size, body_pos, Color::dwhite_lblack(theme),
                       description.c_str(), nullptr, wrap_w);
}

void PlannerPanel::DrawTaskList(const UiScale& scale, bool theme, float window_height)
{
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 130, 30, 255));
    // Height reaches the real window bottom rather than a fixed 650*scale.
    const float panel_top = 220.0f * scale.y;
    const float panel_bottom_margin = 10.0f * scale.y;
    const float panel_h = std::max(200.0f * scale.y, window_height - panel_top - panel_bottom_margin);
    BeginFixedPanel("MissionPlannerPanel", ImVec2(70 * scale.x, panel_top), ImVec2(450 * scale.x, panel_h),
                     scale, theme, 0, ImVec2(8, 8));

    // Fetched fresh further down too -- Clear can destroy the root this same frame.
    size_t count = 0;
    {
        const PlanNode *root_for_count = planner_.plan().root.get();
        if (root_for_count && root_for_count->kind() == NodeKind::Sequence) {
            count = static_cast<const SequenceNode *>(root_for_count)->children.size();
        }
    }

    // Computed here so the status pill can summarize it even when the list is empty.
    const std::vector<Issue> issues = planner_.local_issues();

    // --- Header: title + name, sized to leave room for the pills ---------
    const float header_top_y = ImGui::GetCursorPosY();

    char count_text[24];
    std::snprintf(count_text, sizeof(count_text), "%zu item%s", count, count == 1 ? "" : "s");
    const ImVec2 count_text_size = ImGui::CalcTextSize(count_text);
    const float pill_w = count_text_size.x + 20.0f * scale.x;
    const float pill_h = count_text_size.y + 6.0f * scale.y;
    const float pill_x = ImGui::GetWindowContentRegionMax().x - pill_w;

    size_t error_count = 0, warning_count = 0;
    for (const auto &issue : issues) {
        if (issue.severity == Severity::Error) { ++error_count; } else { ++warning_count; }
    }
    char status_text[96];
    if (error_count > 0 && warning_count > 0) {
        std::snprintf(status_text, sizeof(status_text), "%zu error%s, %zu warning%s",
            error_count, error_count == 1 ? "" : "s", warning_count, warning_count == 1 ? "" : "s");
    } else if (error_count > 0) {
        std::snprintf(status_text, sizeof(status_text), "%zu error%s", error_count, error_count == 1 ? "" : "s");
    } else if (warning_count > 0) {
        std::snprintf(status_text, sizeof(status_text), "%zu warning%s", warning_count, warning_count == 1 ? "" : "s");
    } else {
        std::snprintf(status_text, sizeof(status_text), "No issues");
    }
    const bool status_has_issues = error_count > 0 || warning_count > 0;
    const ImU32 status_bg = error_count > 0   ? IM_COL32(140, 40, 40, 255)
                           : warning_count > 0 ? IM_COL32(150, 110, 20, 255)
                                                : Color::panelBorder(theme);
    const ImVec2 status_label_size = ImGui::CalcTextSize(status_text);
    const float status_w = status_label_size.x + 20.0f * scale.x;
    const float status_h = status_label_size.y + 6.0f * scale.y;
    const float status_x = pill_x - 8.0f * scale.x - status_w;
    const float pills_left_x = (count > 0) ? status_x : pill_x;

    ImGui::PushFont(winInit.getFont(24));  // bold 24
    const float title_h = ImGui::GetFontSize();
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::TextUnformatted("MISSION PLAN");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    const float title_end_x = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;  // screen -> window-local
    ImGui::SameLine();
    ImGui::SetCursorPosY(header_top_y + (title_h - ImGui::GetFontSize()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, Color::dwhite_lblack(theme));
    const std::string plan_name = planner_.plan_name().empty() ? "untitled" : planner_.plan_name();
    const float prefix_w = ImGui::CalcTextSize("- ").x;
    const float name_budget = std::max(0.0f,
        pills_left_x - 8.0f * scale.x - title_end_x - ImGui::GetStyle().ItemSpacing.x - prefix_w);
    ImGui::Text("- %s", TruncateToWidth(plan_name, name_budget).c_str());
    ImGui::PopStyleColor();
    const float header_center_y = header_top_y + title_h * 0.5f;

    // --- Status pill: issue counts, hover for the full list --------------
    ImDrawList *header_draw_list = ImGui::GetWindowDrawList();
    if (count > 0) {
        ImGui::SetCursorPos(ImVec2(status_x, header_center_y - status_h * 0.5f));
        const ImVec2 status_screen_pos = ImGui::GetCursorScreenPos();
        header_draw_list->AddRectFilled(status_screen_pos, ImVec2(status_screen_pos.x + status_w, status_screen_pos.y + status_h),
                                        status_bg, status_h * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(status_screen_pos.x + 10.0f * scale.x, status_screen_pos.y + 3.0f * scale.y));
        ImGui::PushStyleColor(ImGuiCol_Text, status_has_issues ? IM_COL32(255, 255, 255, 255) : Color::white_black(theme));
        ImGui::TextUnformatted(status_text);
        ImGui::PopStyleColor();

        if (ImGui::IsMouseHoveringRect(status_screen_pos, ImVec2(status_screen_pos.x + status_w, status_screen_pos.y + status_h))) {
            ImGui::BeginTooltip();
            if (!status_has_issues) {
                ImGui::TextUnformatted("No issues");
            }
            for (const auto &issue : issues) {
                const ImVec4 color = issue.severity == Severity::Error
                    ? ImVec4(1.0f, 0.36f, 0.36f, 1.0f) : ImVec4(0.96f, 0.78f, 0.30f, 1.0f);
                ImGui::TextColored(color, "%s: %s", ReadableIssuePath(issue.path).c_str(), issue.message.c_str());
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::SetCursorPos(ImVec2(pill_x, header_center_y - pill_h * 0.5f));
    {
        const ImVec2 screen_pos = ImGui::GetCursorScreenPos();
        header_draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + pill_w, screen_pos.y + pill_h),
                                 Color::panelBorder(theme), pill_h * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + 10.0f * scale.x, screen_pos.y + 3.0f * scale.y));
        ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
        ImGui::TextUnformatted(count_text);
        ImGui::PopStyleColor();
    }

    // Pinned explicitly -- the pills above were placed via manual SetCursorPos.
    ImGui::SetCursorPosY(header_top_y + title_h + 8.0f * scale.y);
    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::Separator();
    ImGui::PopStyleColor();

    const PlanNode *root = planner_.plan().root.get();
    const SequenceNode *sequence = (root && root->kind() == NodeKind::Sequence)
        ? static_cast<const SequenceNode *>(root) : nullptr;

    // Rows scroll in their own child, leaving room below for the footer.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale.x, 10.0f * scale.y));
    const float footer_h = ImGui::GetFrameHeightWithSpacing() + 16.0f * scale.y;
    ImGui::PopStyleVar();
    const float rows_h = std::max(0.0f, ImGui::GetContentRegionAvail().y - footer_h);
    ImGui::BeginChild("TaskRowsScroll", ImVec2(0, rows_h), false);

    // --- Rows: numbered badge + skill name/subtitle, or a run_until group --
    if (!sequence || sequence->children.empty()) {
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color::dwhite_lblack(theme));
        ImGui::TextDisabled("No tasks yet -- pick a skill above to get started.");
        ImGui::PopStyleColor();
    } else {
        const float row_width = ImGui::GetContentRegionAvail().x;
        const VehicleCapabilities *task_capabilities = planner_.selected_capabilities();

        // Generic over where values come from/are written to -- reused for top-level and nested tasks.
        auto draw_param_editor = [&](const nlohmann::json &params, const std::map<std::string, ParamSpec> &param_specs,
                                      const std::function<void(const std::string &, const nlohmann::json &)> &set_param) {
            for (const auto &[param_name, param_spec] : param_specs) {
                // No current skill uses polygon params; needs its own vertex-list editor eventually.
                if (param_spec.type == "polygon") {
                    ImGui::TextDisabled("%s: editing not supported yet", param_name.c_str());
                    continue;
                }

                ImGui::PushID(param_name.c_str());
                if (param_spec.type == "bool") {
                    bool value = params.value(param_name, false);
                    if (ImGui::Checkbox(param_name.c_str(), &value)) {
                        set_param(param_name, value);
                    }
                } else if (param_spec.type == "string") {
                    const std::string current = params.value(param_name, std::string());
                    char buffer[128];
                    std::snprintf(buffer, sizeof(buffer), "%s", current.c_str());
                    ImGui::SetNextItemWidth(140.0f * scale.x);
                    if (ImGui::InputText(param_name.c_str(), buffer, sizeof(buffer))) {
                        set_param(param_name, std::string(buffer));
                    }
                } else if (param_spec.type == "point") {
                    const auto current = params.value(param_name, std::vector<double>{0.0, 0.0, 0.0});
                    float xyz[3] = {
                        static_cast<float>(current.size() > 0 ? current[0] : 0.0),
                        static_cast<float>(current.size() > 1 ? current[1] : 0.0),
                        static_cast<float>(current.size() > 2 ? current[2] : 0.0),
                    };
                    ImGui::SetNextItemWidth(220.0f * scale.x);
                    if (ImGui::InputFloat3(param_name.c_str(), xyz)) {
                        set_param(param_name, nlohmann::json::array({xyz[0], xyz[1], xyz[2]}));
                    }
                } else if (param_spec.type == "int") {
                    int value = params.value(param_name, 0);
                    ImGui::SetNextItemWidth(140.0f * scale.x);
                    if (ImGui::InputInt(param_name.c_str(), &value)) {
                        set_param(param_name, value);
                    }
                } else {
                    float value = static_cast<float>(params.value(param_name, 0.0));
                    ImGui::SetNextItemWidth(140.0f * scale.x);
                    if (ImGui::InputFloat(param_name.c_str(), &value)) {
                        set_param(param_name, static_cast<double>(value));
                    }
                }
                ImGui::PopID();
            }
        };

        // Deferred until after the loop -- applying mid-loop would invalidate the indices below.
        int remove_index = -1;
        int move_up_index = -1;
        int move_down_index = -1;
        std::vector<size_t> wrap_indices;
        NodeKind wrap_kind = NodeKind::Task;  // only meaningful when wrap_indices is non-empty
        int ungroup_index = -1;
        bool first_row = true;

        // --- Selection toolbar: appears once 2+ plain tasks are ctrl-selected
        if (!selected_task_indices_.empty()) {
            std::vector<size_t> sorted_selection(selected_task_indices_.begin(), selected_task_indices_.end());
            std::sort(sorted_selection.begin(), sorted_selection.end());
            bool contiguous = true;
            for (size_t k = 1; k < sorted_selection.size(); ++k) {
                if (sorted_selection[k] != sorted_selection[k - 1] + 1) { contiguous = false; break; }
            }
            const bool can_wrap = contiguous && sorted_selection.size() >= 2;

            ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
            ImGui::Text("%zu selected", sorted_selection.size());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::BeginDisabled(!can_wrap);
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button("Run Until")) {
                wrap_indices = sorted_selection;
                wrap_kind = NodeKind::RunUntil;
            }
            PopThemedButtonStyle();
            ImGui::SameLine();
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button("Repeat")) {
                wrap_indices = sorted_selection;
                wrap_kind = NodeKind::Repeat;
            }
            PopThemedButtonStyle();
            ImGui::SameLine();
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button("Retry")) {
                wrap_indices = sorted_selection;
                wrap_kind = NodeKind::Retry;
            }
            PopThemedButtonStyle();
            ImGui::EndDisabled();
            ImGui::SameLine();
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button("Cancel")) {
                selected_task_indices_.clear();
            }
            PopThemedButtonStyle();
            if (!contiguous) {
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color::dwhite_lblack(theme));
                ImGui::TextDisabled("Select adjacent tasks only");
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();
        }

        for (size_t i = 0; i < sequence->children.size(); ++i) {
            const auto &child = sequence->children[i];
            const int display_index = static_cast<int>(i) + 1;
            const std::string node_path = "root.children[" + std::to_string(i) + "]";

            // Must run before any manual SetCursorScreenPos below -- SetScrollHereY reads the auto-layout cursor.
            if (scroll_to_expanded_ && expanded_task_index_ == static_cast<int>(i)) {
                ImGui::SetScrollHereY(0.2f);
                scroll_to_expanded_ = false;
            }

            if (!first_row) {
                const ImVec2 sep_pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(sep_pos.x, sep_pos.y - 5.0f * scale.y),
                    ImVec2(sep_pos.x + row_width, sep_pos.y - 5.0f * scale.y),
                    Color::panelBorder(theme), 1.0f * scale.uniform());
            }
            first_row = false;

            bool has_error = false;
            bool has_warning = false;
            for (const auto &issue : issues) {
                if (issue.path == node_path || issue.path.starts_with(node_path + ".")) {
                    if (issue.severity == Severity::Error) { has_error = true; }
                    else { has_warning = true; }
                }
            }

            if (child->kind() == NodeKind::RunUntil || child->kind() == NodeKind::Repeat || child->kind() == NodeKind::Retry) {
                const NodeKind wrapper_kind = child->kind();
                const auto *run_until = wrapper_kind == NodeKind::RunUntil ? static_cast<const RunUntilNode *>(child.get()) : nullptr;
                const auto *repeat    = wrapper_kind == NodeKind::Repeat   ? static_cast<const RepeatNode *>(child.get())   : nullptr;
                const auto *retry     = wrapper_kind == NodeKind::Retry    ? static_cast<const RetryNode *>(child.get())    : nullptr;
                const PlanNode *inner_child = run_until ? run_until->child.get() : repeat ? repeat->child.get() : retry->child.get();

                const char *label = run_until ? "RUN UNTIL" : repeat ? "REPEAT" : "RETRY";
                const ImU32 accent = run_until ? IM_COL32(255, 130, 30, 255)    // orange
                                    : repeat   ? IM_COL32(90, 150, 255, 255)    // blue
                                               : IM_COL32(190, 130, 255, 255);  // purple

                std::string subtitle;
                if (run_until) {
                    subtitle = WrapperSubtitle(WrapperChildTaskCount(inner_child), ConditionsSummary(*run_until));
                } else if (repeat) {
                    subtitle = WrapperSubtitle(WrapperChildTaskCount(inner_child), "repeat " + std::to_string(repeat->count) + "x");
                } else {
                    subtitle = WrapperSubtitle(WrapperChildTaskCount(inner_child), "retry up to " + std::to_string(retry->max_attempts) + "x");
                }

                const ImU32 status_color = has_error   ? IM_COL32(255, 92, 92, 255)
                                          : has_warning ? IM_COL32(245, 200, 76, 255)
                                                        : accent;

                const ImVec2 row_start = ImGui::GetCursorScreenPos();
                const float badge_radius = 12.0f * scale.uniform();
                const float badge_diameter = badge_radius * 2.0f;
                const ImVec2 badge_center(row_start.x + badge_radius, row_start.y + badge_radius);

                const float button_size = ImGui::GetFrameHeight();
                const float buttons_w = button_size * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                const float content_w = row_width - buttons_w - 8.0f * scale.x;

                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddCircle(badge_center, badge_radius, status_color, 0, 1.5f * scale.uniform());
                char index_label[16];
                std::snprintf(index_label, sizeof(index_label), "%d", display_index);
                const ImVec2 index_size = ImGui::CalcTextSize(index_label);
                draw_list->AddText(ImVec2(badge_center.x - index_size.x * 0.5f, badge_center.y - index_size.y * 0.5f),
                                   Color::white_black(theme), index_label);

                ImGui::SetCursorScreenPos(ImVec2(row_start.x + badge_diameter + 12.0f * scale.x, row_start.y));
                ImGui::BeginGroup();
                ImGui::PushFont(winInit.getFont(181));
                const float title_line_h = ImGui::GetFontSize();
                ImGui::PushStyleColor(ImGuiCol_Text, has_error || has_warning ? status_color : accent);
                ImGui::TextUnformatted(label);
                ImGui::PopStyleColor();
                ImGui::PopFont();
                ImGui::PushStyleColor(ImGuiCol_Text, has_error || has_warning ? status_color : Color::dwhite_lblack(theme));
                ImGui::TextUnformatted(subtitle.c_str());
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                const float text_height = ImGui::GetItemRectSize().y;
                const float row_height = std::max(badge_diameter, text_height);
                // Aligned to the title line only, so buttons don't drift as the subtitle grows longer.
                const float button_align_h = std::max(badge_diameter, title_line_h);

                const bool content_hovered = ImGui::IsMouseHoveringRect(
                    row_start, ImVec2(row_start.x + content_w, row_start.y + row_height));
                if (content_hovered) {
                    draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                             IM_COL32(255, 255, 255, 15), 6.0f * scale.uniform());
                }
                if (content_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    expanded_task_index_ = (expanded_task_index_ == static_cast<int>(i)) ? -1 : static_cast<int>(i);
                    expanded_group_task_index_ = -1;
                }

                ImGui::SetCursorScreenPos(ImVec2(row_start.x + row_width - buttons_w,
                                                 row_start.y + (button_align_h - button_size) * 0.5f));
                ImGui::PushID(static_cast<int>(i));
                PushThemedButtonStyle(theme, false);
                if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { move_up_index = static_cast<int>(i); }
                PopThemedButtonStyle();
                ImGui::SameLine();
                PushThemedButtonStyle(theme, false);
                if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { move_down_index = static_cast<int>(i); }
                PopThemedButtonStyle();
                ImGui::SameLine();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
                if (ImGui::Button("X", ImVec2(button_size, button_size))) { remove_index = static_cast<int>(i); }
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar();
                ImGui::PopID();

                ImGui::SetCursorScreenPos(row_start);
                ImGui::Dummy(ImVec2(row_width, row_height + 10.0f * scale.y));

                if (expanded_task_index_ == static_cast<int>(i)) {
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::Indent(badge_diameter + 12.0f * scale.x);
                    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));

                    if (run_until) {
                    ImGui::PushStyleColor(ImGuiCol_Button, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 130, 30, 255));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 130, 30, 180));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(255, 130, 30, 255));

                    const std::vector<std::string> empty_conditions;
                    const std::vector<std::string> &available_conditions =
                        task_capabilities ? task_capabilities->conditions : empty_conditions;
                    static const char *kOps[] = {"(none)", ">=", "<=", ">", "<", "=="};

                    int remove_condition_index = -1;
                    for (size_t c = 0; c < run_until->conditions_any.size(); ++c) {
                        const Condition &condition = run_until->conditions_any[c];
                        ImGui::PushID(static_cast<int>(c));

                        ImGui::SetNextItemWidth(140.0f * scale.x);
                        if (ImGui::BeginCombo("##Cond", condition.cond.c_str())) {
                            for (const auto &name : available_conditions) {
                                if (ImGui::Selectable(name.c_str(), name == condition.cond)) {
                                    Condition updated = condition;
                                    updated.cond = name;
                                    planner_.set_group_condition(i, c, updated);
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();

                        const std::string current_op = condition.op.value_or("(none)");
                        ImGui::SetNextItemWidth(70.0f * scale.x);
                        if (ImGui::BeginCombo("##Op", current_op.c_str())) {
                            for (const char *op : kOps) {
                                if (ImGui::Selectable(op, current_op == op)) {
                                    Condition updated = condition;
                                    if (std::string(op) == "(none)") {
                                        updated.op.reset();
                                        updated.value.reset();
                                    } else {
                                        updated.op = op;
                                        if (!updated.value) { updated.value = 0.0; }
                                    }
                                    planner_.set_group_condition(i, c, updated);
                                }
                            }
                            ImGui::EndCombo();
                        }

                        if (condition.op) {
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(90.0f * scale.x);
                            if (ConditionValueIsInt(condition.cond)) {
                                int value = static_cast<int>(condition.value.value_or(0.0));
                                if (ImGui::InputInt("##Value", &value)) {
                                    Condition updated = condition;
                                    updated.value = static_cast<double>(value);
                                    planner_.set_group_condition(i, c, updated);
                                }
                            } else {
                                float value = static_cast<float>(condition.value.value_or(0.0));
                                if (ImGui::InputFloat("##Value", &value)) {
                                    Condition updated = condition;
                                    updated.value = static_cast<double>(value);
                                    planner_.set_group_condition(i, c, updated);
                                }
                            }
                        }

                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
                        if (ImGui::Button("X")) { remove_condition_index = static_cast<int>(c); }
                        ImGui::PopStyleColor(4);

                        ImGui::PopID();
                    }
                    if (remove_condition_index >= 0) {
                        planner_.remove_group_condition(i, static_cast<size_t>(remove_condition_index));
                    }

                    PushThemedButtonStyle(theme, false);
                    if (ImGui::Button("+ Add condition")) {
                        planner_.add_group_condition(i);
                    }
                    PopThemedButtonStyle();
                    ImGui::SameLine();

                    ImGui::PopStyleColor(5);  // Button/ButtonHovered/Header/HeaderHovered/HeaderActive
                    } else if (repeat) {
                        int count = repeat->count;
                        ImGui::SetNextItemWidth(90.0f * scale.x);
                        // step=0 disables the +/- buttons.
                        if (ImGui::InputInt("Count", &count, 0, 0)) {
                            planner_.set_repeat_count(i, count);
                        }
                        ImGui::SameLine();
                    } else {
                        int max_attempts = retry->max_attempts;
                        ImGui::SetNextItemWidth(90.0f * scale.x);
                        if (ImGui::InputInt("Max attempts", &max_attempts, 0, 0)) {
                            planner_.set_retry_max_attempts(i, max_attempts);
                        }
                        ImGui::SameLine();
                    }

                    PushThemedButtonStyle(theme, false);
                    if (ImGui::Button("Ungroup")) {
                        ungroup_index = static_cast<int>(i);
                    }
                    PopThemedButtonStyle();

                    ImGui::PopStyleColor(4);  // Text/FrameBg/FrameBgHovered/FrameBgActive

                    // --- Nested tasks -------------------------------------
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                    ImGui::Indent(16.0f * scale.x);

                    if (inner_child && inner_child->kind() == NodeKind::Sequence) {
                        const auto &inner = static_cast<const SequenceNode &>(*inner_child);
                        // Deferred, same reasoning as the top-level loop.
                        int nested_remove_index = -1;
                        int nested_move_up_index = -1;
                        int nested_move_down_index = -1;

                        for (size_t t = 0; t < inner.children.size(); ++t) {
                            if (inner.children[t]->kind() != NodeKind::Task) { continue; }
                            const auto &nested_task = static_cast<const TaskNode &>(*inner.children[t]);
                            const std::string nested_path = node_path + ".child.children[" + std::to_string(t) + "]";

                            bool nested_error = false, nested_warning = false;
                            for (const auto &issue : issues) {
                                if (issue.path == nested_path || issue.path.starts_with(nested_path + ".")) {
                                    if (issue.severity == Severity::Error) { nested_error = true; }
                                    else { nested_warning = true; }
                                }
                            }
                            const ImU32 nested_color = nested_error   ? IM_COL32(255, 92, 92, 255)
                                                      : nested_warning ? IM_COL32(245, 200, 76, 255)
                                                                       : Color::white_black(theme);

                            const SkillSpec *nested_spec = nullptr;
                            if (task_capabilities) {
                                auto it = task_capabilities->skills.find(nested_task.skill);
                                if (it != task_capabilities->skills.end()) { nested_spec = &it->second; }
                            }
                            const bool nested_has_params = nested_spec && !nested_spec->params.empty();

                            ImGui::PushID(static_cast<int>(t));

                            const ImVec2 nested_row_start = ImGui::GetCursorScreenPos();
                            const float nested_row_width = ImGui::GetContentRegionAvail().x;
                            const float nested_button_size = ImGui::GetFrameHeight();
                            const float nested_buttons_w = nested_button_size * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                            const float nested_content_w = nested_row_width - nested_buttons_w - 8.0f * scale.x;

                            ImGui::BeginGroup();
                            ImGui::PushStyleColor(ImGuiCol_Text, nested_color);
                            ImGui::Text("%d. %s", static_cast<int>(t) + 1, ToUpper(nested_task.skill).c_str());
                            ImGui::PopStyleColor();
                            ImGui::PushStyleColor(ImGuiCol_Text, Color::dwhite_lblack(theme));
                            ImGui::TextUnformatted(TaskSubtitle(nested_task).c_str());
                            ImGui::PopStyleColor();
                            ImGui::EndGroup();
                            const float nested_row_height = ImGui::GetItemRectSize().y;

                            // Gated on nested_spec existing (recognized skill), not on it having params.
                            const bool nested_hovered = nested_spec && ImGui::IsMouseHoveringRect(
                                nested_row_start, ImVec2(nested_row_start.x + nested_content_w, nested_row_start.y + nested_row_height));
                            if (nested_hovered) {
                                ImGui::GetWindowDrawList()->AddRectFilled(
                                    nested_row_start, ImVec2(nested_row_start.x + nested_content_w, nested_row_start.y + nested_row_height),
                                    IM_COL32(255, 255, 255, 15), 4.0f * scale.uniform());
                            }
                            if (nested_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                expanded_group_task_index_ =
                                    (expanded_group_task_index_ == static_cast<int>(t)) ? -1 : static_cast<int>(t);
                            }

                            // Up/down/remove, right-aligned within the nested row.
                            ImGui::SetCursorScreenPos(ImVec2(nested_row_start.x + nested_row_width - nested_buttons_w,
                                                             nested_row_start.y + (nested_row_height - nested_button_size) * 0.5f));
                            PushThemedButtonStyle(theme, false);
                            if (ImGui::ArrowButton("##nup", ImGuiDir_Up)) { nested_move_up_index = static_cast<int>(t); }
                            PopThemedButtonStyle();
                            ImGui::SameLine();
                            PushThemedButtonStyle(theme, false);
                            if (ImGui::ArrowButton("##ndown", ImGuiDir_Down)) { nested_move_down_index = static_cast<int>(t); }
                            PopThemedButtonStyle();
                            ImGui::SameLine();
                            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
                            if (ImGui::Button("X", ImVec2(nested_button_size, nested_button_size))) { nested_remove_index = static_cast<int>(t); }
                            ImGui::PopStyleColor(4);
                            ImGui::PopStyleVar();

                            ImGui::SetCursorScreenPos(nested_row_start);
                            ImGui::Dummy(ImVec2(nested_row_width, nested_row_height));

                            if (nested_has_params && expanded_group_task_index_ == static_cast<int>(t)) {
                                ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
                                ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
                                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
                                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));
                                ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 130, 30, 255));
                                ImGui::Indent(12.0f * scale.x);
                                draw_param_editor(nested_task.params, nested_spec->params,
                                    [&](const std::string &param_name, const nlohmann::json &value) {
                                        planner_.set_group_task_param(i, t, param_name, value);
                                    });
                                ImGui::Unindent(12.0f * scale.x);
                                ImGui::PopStyleColor(5);
                            }
                            ImGui::PopID();
                            ImGui::Spacing();
                        }

                        // Removing the last task dissolves the whole group -- clear both indices.
                        if (nested_remove_index >= 0) {
                            const bool group_will_dissolve = inner.children.size() <= 1;
                            planner_.remove_group_task(i, static_cast<size_t>(nested_remove_index));
                            if (group_will_dissolve) {
                                expanded_task_index_ = -1;
                                expanded_group_task_index_ = -1;
                            } else if (nested_remove_index == expanded_group_task_index_) {
                                expanded_group_task_index_ = -1;
                            } else if (nested_remove_index < expanded_group_task_index_) {
                                --expanded_group_task_index_;
                            }
                        } else if (nested_move_up_index >= 0) {
                            planner_.move_group_task_up(i, static_cast<size_t>(nested_move_up_index));
                            if (expanded_group_task_index_ == nested_move_up_index) {
                                expanded_group_task_index_ = nested_move_up_index - 1;
                            } else if (expanded_group_task_index_ == nested_move_up_index - 1) {
                                expanded_group_task_index_ = nested_move_up_index;
                            }
                        } else if (nested_move_down_index >= 0) {
                            planner_.move_group_task_down(i, static_cast<size_t>(nested_move_down_index));
                            if (expanded_group_task_index_ == nested_move_down_index) {
                                expanded_group_task_index_ = nested_move_down_index + 1;
                            } else if (expanded_group_task_index_ == nested_move_down_index + 1) {
                                expanded_group_task_index_ = nested_move_down_index;
                            }
                        }
                    }

                    ImGui::Unindent(16.0f * scale.x);
                    ImGui::Unindent(badge_diameter + 12.0f * scale.x);
                    ImGui::Spacing();
                    ImGui::PopID();
                }
                continue;
            }

            if (child->kind() != NodeKind::Task) { continue; }
            const auto &task = static_cast<const TaskNode &>(*child);

            const SkillSpec *spec = nullptr;
            if (task_capabilities) {
                auto skill_it = task_capabilities->skills.find(task.skill);
                if (skill_it != task_capabilities->skills.end()) { spec = &skill_it->second; }
            }
            const bool is_selected = selected_task_indices_.count(i) > 0;

            const ImU32 status_color = has_error   ? IM_COL32(255, 92, 92, 255)
                                      : has_warning ? IM_COL32(245, 200, 76, 255)
                                                    : Color::panelBorder(theme);

            const ImVec2 row_start = ImGui::GetCursorScreenPos();
            const float badge_radius = 12.0f * scale.uniform();
            const float badge_diameter = badge_radius * 2.0f;
            const ImVec2 badge_center(row_start.x + badge_radius, row_start.y + badge_radius);

            // Hoisted so the hover/click check below can exclude the button strip.
            const float button_size = ImGui::GetFrameHeight();
            const float buttons_w = button_size * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
            const float content_w = row_width - buttons_w - 8.0f * scale.x;

            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCircle(badge_center, badge_radius, status_color, 0, 1.5f * scale.uniform());
            char index_label[16];
            std::snprintf(index_label, sizeof(index_label), "%d", display_index);
            const ImVec2 index_size = ImGui::CalcTextSize(index_label);
            draw_list->AddText(ImVec2(badge_center.x - index_size.x * 0.5f, badge_center.y - index_size.y * 0.5f),
                               Color::white_black(theme), index_label);

            ImGui::SetCursorScreenPos(ImVec2(row_start.x + badge_diameter + 12.0f * scale.x, row_start.y));
            ImGui::BeginGroup();
            ImGui::PushFont(winInit.getFont(181));
            ImGui::PushStyleColor(ImGuiCol_Text, has_error || has_warning ? status_color : Color::white_black(theme));
            ImGui::TextUnformatted(ToUpper(task.skill).c_str());
            ImGui::PopStyleColor();
            ImGui::PopFont();
            ImGui::PushStyleColor(ImGuiCol_Text, has_error || has_warning ? status_color : Color::dwhite_lblack(theme));
            ImGui::TextUnformatted(TaskSubtitle(task).c_str());
            ImGui::PopStyleColor();
            ImGui::EndGroup();
            const float text_height = ImGui::GetItemRectSize().y;
            const float row_height = std::max(badge_diameter, text_height);

            // Manual rect check -- a "row" here is badge + text + buttons, not one ImGui item.
            const bool content_hovered = ImGui::IsMouseHoveringRect(
                row_start, ImVec2(row_start.x + content_w, row_start.y + row_height));
            if (is_selected) {
                draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                         IM_COL32(255, 130, 30, 35), 6.0f * scale.uniform());
            } else if (content_hovered) {
                draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                         IM_COL32(255, 255, 255, 15), 6.0f * scale.uniform());
            }
            if (content_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (ImGui::GetIO().KeyCtrl) {
                    if (is_selected) { selected_task_indices_.erase(i); }
                    else { selected_task_indices_.insert(i); }
                } else if (spec) {
                    // Gated on spec existing, not on it having params -- a param-less task must stay selectable too.
                    expanded_task_index_ = (expanded_task_index_ == static_cast<int>(i)) ? -1 : static_cast<int>(i);
                }
            }

            // Up/down/remove, right-aligned within the row.
            ImGui::SetCursorScreenPos(ImVec2(row_start.x + row_width - buttons_w,
                                             row_start.y + (row_height - button_size) * 0.5f));
            ImGui::PushID(static_cast<int>(i));
            PushThemedButtonStyle(theme, false);
            if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { move_up_index = static_cast<int>(i); }
            PopThemedButtonStyle();
            ImGui::SameLine();
            PushThemedButtonStyle(theme, false);
            if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { move_down_index = static_cast<int>(i); }
            PopThemedButtonStyle();
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            // Always white -- this button's background is a fixed dark red regardless of theme.
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
            if (ImGui::Button("X", ImVec2(button_size, button_size))) { remove_index = static_cast<int>(i); }
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
            ImGui::PopID();

            // Dummy() reserves the row's space -- a manual SetCursorScreenPos alone doesn't extend the content region.
            ImGui::SetCursorScreenPos(row_start);
            ImGui::Dummy(ImVec2(row_width, row_height + 10.0f * scale.y));

            // spec re-checked here too -- SelectTask() (a map-marker click) can bypass the click-site gate above.
            if (expanded_task_index_ == static_cast<int>(i) && spec) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::Indent(badge_diameter + 12.0f * scale.x);
                ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 130, 30, 255));

                draw_param_editor(task.params, spec->params,
                    [&](const std::string &param_name, const nlohmann::json &value) {
                        planner_.set_task_param(i, param_name, value);
                    });

                ImGui::PopStyleColor(5);
                ImGui::Unindent(badge_diameter + 12.0f * scale.x);
                ImGui::Spacing();
                ImGui::PopID();
            }
        }

        // Remap expanded_task_index_ through the shift; selection is simpler to just drop than remap.
        if (remove_index >= 0) {
            planner_.remove_task(static_cast<size_t>(remove_index));
            if (remove_index == expanded_task_index_) {
                expanded_task_index_ = -1;
            } else if (remove_index < expanded_task_index_) {
                --expanded_task_index_;
            }
            selected_task_indices_.clear();
        } else if (move_up_index >= 0) {
            planner_.move_task_up(static_cast<size_t>(move_up_index));
            if (expanded_task_index_ == move_up_index) {
                expanded_task_index_ = move_up_index - 1;
            } else if (expanded_task_index_ == move_up_index - 1) {
                expanded_task_index_ = move_up_index;
            }
            selected_task_indices_.clear();
        } else if (move_down_index >= 0) {
            planner_.move_task_down(static_cast<size_t>(move_down_index));
            if (expanded_task_index_ == move_down_index) {
                expanded_task_index_ = move_down_index + 1;
            } else if (expanded_task_index_ == move_down_index + 1) {
                expanded_task_index_ = move_down_index;
            }
            selected_task_indices_.clear();
        } else if (!wrap_indices.empty()) {
            if (wrap_kind == NodeKind::RunUntil) {
                planner_.wrap_in_run_until(wrap_indices);
            } else if (wrap_kind == NodeKind::Repeat) {
                planner_.wrap_in_repeat(wrap_indices, 2);
            } else if (wrap_kind == NodeKind::Retry) {
                planner_.wrap_in_retry(wrap_indices, 3);
            }
            selected_task_indices_.clear();
            expanded_task_index_ = -1;
        } else if (ungroup_index >= 0) {
            planner_.ungroup(static_cast<size_t>(ungroup_index));
            expanded_task_index_ = -1;
            expanded_group_task_index_ = -1;
        }
    }

    ImGui::EndChild();

    // Footer: always visible below the scrolling rows.
    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::Separator();
    ImGui::PopStyleColor();

    // Centered in the space left below the divider, rather than hugging its top.
    const float button_h = ImGui::GetFontSize() + 20.0f * scale.y;  // matches the FramePadding pushed below
    const float band_remaining = ImGui::GetContentRegionAvail().y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, (band_remaining - button_h) * 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale.x, 10.0f * scale.y));

    // --- Upload: publish the plan, track the vehicle's validation of it --
    {
        const bool has_tasks = planner_.has_tasks();
        // Button itself previews local plan validity -- red/yellow before you even click.
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
            planner_.upload();
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

        // Compact result indicator: check for clean, cross for errors/timeout, hollow ring while waiting.
        const UploadStatus upload_status = planner_.upload_status();
        const float icon_span = ImGui::GetFrameHeight();
        ImGui::Dummy(ImVec2(icon_span, icon_span));
        const ImVec2 icon_min = ImGui::GetItemRectMin();
        const ImVec2 icon_max = ImGui::GetItemRectMax();
        const ImVec2 icon_center((icon_min.x + icon_max.x) * 0.5f, (icon_min.y + icon_max.y) * 0.5f);
        const float icon_size = icon_span * 0.55f;
        ImDrawList *footer_draw_list = ImGui::GetWindowDrawList();
        // Boxed like the buttons it sits next to, or it reads as oddly spaced.
        footer_draw_list->AddRectFilled(icon_min, icon_max, Color::panelBorder(theme), 12.0f);

        std::string tooltip_header;
        std::vector<Issue> upload_issues;
        switch (upload_status) {
            case UploadStatus::Idle:
                // Dim dash rather than an empty box -- reads as "nothing yet", not a glitch.
                footer_draw_list->AddLine(ImVec2(icon_center.x - icon_size * 0.3f, icon_center.y),
                                          ImVec2(icon_center.x + icon_size * 0.3f, icon_center.y),
                                          Color::dwhite_lblack(theme), 2.5f * scale.uniform());
                tooltip_header = "Not uploaded yet";
                break;
            case UploadStatus::Uploading:
                footer_draw_list->AddCircle(icon_center, icon_size * 0.5f, IM_COL32(245, 200, 76, 255), 0, 2.0f * scale.uniform());
                tooltip_header = "Uploading -- waiting for vehicle...";
                break;
            case UploadStatus::TimedOut:
                DrawCross(footer_draw_list, icon_center, icon_size, IM_COL32(255, 92, 92, 255), 2.5f * scale.uniform());
                tooltip_header = "No response from vehicle -- click Upload to retry";
                break;
            case UploadStatus::Validated: {
                upload_issues = planner_.upload_issues();
                size_t up_errors = 0, up_warnings = 0;
                for (const auto &issue : upload_issues) {
                    if (issue.severity == Severity::Error) { ++up_errors; } else { ++up_warnings; }
                }
                if (up_errors > 0) {
                    DrawCross(footer_draw_list, icon_center, icon_size, IM_COL32(255, 92, 92, 255), 2.5f * scale.uniform());
                    tooltip_header = std::to_string(up_errors) + (up_errors == 1 ? " error" : " errors") + " on vehicle";
                } else if (up_warnings > 0) {
                    DrawCheckmark(footer_draw_list, icon_center, icon_size, IM_COL32(245, 200, 76, 255), 2.5f * scale.uniform());
                    tooltip_header = std::to_string(up_warnings) + (up_warnings == 1 ? " warning" : " warnings") + " on vehicle";
                } else {
                    DrawCheckmark(footer_draw_list, icon_center, icon_size, IM_COL32(100, 220, 100, 255), 2.5f * scale.uniform());
                    tooltip_header = "Validated on vehicle -- no issues";
                }
                break;
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tooltip_header.c_str());
            for (const auto &issue : upload_issues) {
                const ImVec4 color = issue.severity == Severity::Error
                    ? ImVec4(1.0f, 0.36f, 0.36f, 1.0f) : ImVec4(0.96f, 0.78f, 0.30f, 1.0f);
                ImGui::TextColored(color, "%s: %s", ReadableIssuePath(issue.path).c_str(), issue.message.c_str());
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::SameLine();
    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Save")) {
        save_load_dialog_.Open(SaveLoadDialog::Mode::Save, PlansDirectory(), ".json",
            [this](const std::string &path) {
                planner_.save(path);
            });
    }
    PopThemedButtonStyle();
    ImGui::SameLine();
    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Load")) {
        save_load_dialog_.Open(SaveLoadDialog::Mode::Load, PlansDirectory(), ".json",
            [this](const std::string &path) {
                planner_.load(path);
                expanded_task_index_ = -1;
                expanded_group_task_index_ = -1;
                selected_task_indices_.clear();
            });
    }
    PopThemedButtonStyle();

    // Right-aligned on the same row as Save/Load, opposite side.
    ImGui::SameLine();
    const float clear_w = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - clear_w);
    // Always white -- this button's background is a fixed dark red regardless of theme.
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
    if (ImGui::Button("Clear")) {
        ImGui::OpenPopup("ConfirmClearPopup");
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();

    // NoTitleBar: BeginPopupModal would otherwise render the internal ID as a literal title-bar caption.
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("ConfirmClearPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("Clear the entire flight plan?");
        ImGui::Spacing();
        // Always white -- overrides the ambient theme-toggled Text color pushed above.
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
        if (ImGui::Button("Yes, clear it")) {
            planner_.clear();
            expanded_task_index_ = -1;
            expanded_group_task_index_ = -1;
            selected_task_indices_.clear();
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

    save_load_dialog_.Draw(scale, theme);

    EndFixedPanel();
    ImGui::PopStyleColor(4);
}
