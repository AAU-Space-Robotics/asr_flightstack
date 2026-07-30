#include "planner_panel.h"

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

// Plans always live here -- Save/Load only ever deal in a name within it,
// never an arbitrary path.
std::string PlansDirectory() {
    return ament_index_cpp::get_package_share_directory("asr_mission") + "/plans";
}

// Matches BeginFixedPanel/CustomButton's own rounding and colors so a plain
// ImGui::Button blends into the app's theme. `active` uses the sidebar's
// orange highlight for "this one is currently selected."
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

// Display-only -- callers still pass the real (lowercase) skill/vehicle
// name to Planner; only the on-screen label gets capitalized.
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

// Ellipsizes to fit max_width -- the header's plan name has no fixed
// length and would otherwise run on underneath the status/item pills
// floating at a fixed position on the right.
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

// Condition has no declared type the way a skill's ParamSpec does (just a
// name string), so there's nowhere to read this from -- probes_found is a
// count and can't be fractional. Belongs in the manifest schema eventually,
// alongside the same kind of stopgap in planner.cpp's DefaultParamValue.
bool ConditionValueIsInt(const std::string &cond) {
    return cond == "probes_found";
}

// The validator reports issues by JSON path (e.g.
// "root.children[2].child.children[0].params.alt"), which is exactly what
// you want for debugging but not for reading in the UI -- translate it into
// the same "Task N" / "Subtask M" language the row numbers already use.
// A group can currently only wrap plain tasks (not another group), so this
// only ever needs to handle one level of nesting, not arbitrary depth.
std::string HumanizeIssuePath(const std::string &path) {
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

// "3 tasks  ·  <detail>" -- shared prefix for any wrapper kind (run_until/
// repeat/retry), which otherwise only differ in what the detail says.
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

} // namespace

PlannerPanel::PlannerPanel(Planner &planner)
    : planner_(planner)
{
}

void PlannerPanel::Draw(float scale, bool theme)
{
    DrawVehicleAndPalette(scale, theme);
    DrawTaskList(scale, theme);
}

void PlannerPanel::DrawVehicleAndPalette(float scale, bool theme)
{
    BeginFixedPanel("VehicleAndPalettePanel", ImVec2(70 * scale, 70 * scale), ImVec2(450 * scale, 140 * scale),
                     scale, theme, 0, ImVec2(8, 8));

    // --- Vehicle dropdown ------------------------------------------------
    const VehicleCapabilities *selected = planner_.selected_capabilities();
    const std::string preview_text = selected ? ToUpper(selected->vehicle) : std::string("SELECT VEHICLE...");

    // BeginCombo's text-preview area (FrameBg/FrameBgHovered) and its arrow
    // box (Button/ButtonHovered) are separate color slots.
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Button, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(255, 130, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(255, 130, 30, 255));
    ImGui::PushFont(winInit.getFont(181));  // bold 18
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
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f * scale, 8.0f * scale));
        for (size_t i = 0; i < skill_names.size(); ++i) {
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button(ToUpper(skill_names[i]).c_str())) {
                planner_.add_task(skill_names[i]);
            }
            PopThemedButtonStyle();

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
    }

    EndFixedPanel();
}

void PlannerPanel::DrawTaskList(float scale, bool theme)
{
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(255, 130, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(255, 130, 30, 255));
    BeginFixedPanel("MissionPlannerPanel", ImVec2(70 * scale, 220 * scale), ImVec2(450 * scale, 650 * scale),
                     scale, theme, 0, ImVec2(8, 8));

    // root/sequence are fetched fresh further down, after the Clear button:
    // Clear can call planner_.clear() this same frame, which destroys the
    // old root, so a pointer captured here would dangle by the time the row
    // loop below reads it.
    size_t count = 0;
    {
        const PlanNode *root_for_count = planner_.plan().root.get();
        if (root_for_count && root_for_count->kind() == NodeKind::Sequence) {
            count = static_cast<const SequenceNode *>(root_for_count)->children.size();
        }
    }

    // Computed once here (not just inside the row loop below) so the
    // header's status pill can summarize it even when the list is empty.
    const std::vector<Issue> issues = planner_.local_issues();

    // --- Header: title + name, sized to leave room for the pills ---------
    // header_top_y/title_h capture the title line's own span so every other
    // element on this row (name, pills) can center itself against that
    // span, rather than each other's differing heights.
    const float header_top_y = ImGui::GetCursorPosY();

    // Pills are computed (not yet drawn) before the name, so the name knows
    // how much room is actually left and can truncate to fit it instead of
    // running on underneath them.
    char count_text[24];
    std::snprintf(count_text, sizeof(count_text), "%zu item%s", count, count == 1 ? "" : "s");
    const ImVec2 count_text_size = ImGui::CalcTextSize(count_text);
    const float pill_w = count_text_size.x + 20.0f * scale;
    const float pill_h = count_text_size.y + 6.0f * scale;
    const float pill_x = ImGui::GetWindowContentRegionMax().x - pill_w;

    // Status pill only renders once the plan has at least one item (see
    // below), so it's only counted as eating into the name's space then.
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
    const float status_w = status_label_size.x + 20.0f * scale;
    const float status_h = status_label_size.y + 6.0f * scale;
    const float status_x = pill_x - 8.0f * scale - status_w;
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
    const std::string plan_name = current_plan_name_.empty() ? "untitled" : current_plan_name_;
    const float prefix_w = ImGui::CalcTextSize("- ").x;
    const float name_budget = std::max(0.0f,
        pills_left_x - 8.0f * scale - title_end_x - ImGui::GetStyle().ItemSpacing.x - prefix_w);
    ImGui::Text("- %s", TruncateToWidth(plan_name, name_budget).c_str());
    ImGui::PopStyleColor();
    const float header_center_y = header_top_y + title_h * 0.5f;

    // --- Status pill: issue counts, hover for the full list --------------
    // Skipped entirely on an empty plan -- structural issues like "no root
    // node" are real, but showing errors before the user has added a
    // single task reads as the tool complaining for no reason.
    ImDrawList *header_draw_list = ImGui::GetWindowDrawList();
    if (count > 0) {
        ImGui::SetCursorPos(ImVec2(status_x, header_center_y - status_h * 0.5f));
        const ImVec2 status_screen_pos = ImGui::GetCursorScreenPos();
        header_draw_list->AddRectFilled(status_screen_pos, ImVec2(status_screen_pos.x + status_w, status_screen_pos.y + status_h),
                                        status_bg, status_h * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(status_screen_pos.x + 10.0f * scale, status_screen_pos.y + 3.0f * scale));
        // White on the colored (red/amber) backgrounds for contrast, same
        // reasoning as Clear's always-white text; themed on the neutral
        // "no issues" background, matching the item-count pill beside it.
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
                ImGui::TextColored(color, "%s: %s", HumanizeIssuePath(issue.path).c_str(), issue.message.c_str());
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::SetCursorPos(ImVec2(pill_x, header_center_y - pill_h * 0.5f));
    {
        const ImVec2 screen_pos = ImGui::GetCursorScreenPos();
        header_draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + pill_w, screen_pos.y + pill_h),
                                 Color::panelBorder(theme), pill_h * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(screen_pos.x + 10.0f * scale, screen_pos.y + 3.0f * scale));
        ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
        ImGui::TextUnformatted(count_text);
        ImGui::PopStyleColor();
    }

    // Clear/pill are placed via manual SetCursorPos, vertically centered on
    // the title -- left alone, the next auto-laid-out item (the separator)
    // would inherit whatever cursor position that manual placement last
    // left behind rather than clearing the taller title's actual bottom,
    // making the gap above the separator uneven. Pin it explicitly instead.
    ImGui::SetCursorPosY(header_top_y + title_h + 8.0f * scale);
    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::Separator();
    ImGui::PopStyleColor();

    const PlanNode *root = planner_.plan().root.get();
    const SequenceNode *sequence = (root && root->kind() == NodeKind::Sequence)
        ? static_cast<const SequenceNode *>(root) : nullptr;

    // Rows scroll in their own child, sized to leave room for the Save/Load
    // footer below -- otherwise the footer would sit at the bottom of the
    // scrollable content and need scrolling to reach once the list grows.
    // Computed with the footer's own (larger) FramePadding so the reserved
    // space matches what the footer actually draws at.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale, 10.0f * scale));
    const float footer_h = ImGui::GetFrameHeightWithSpacing() + 16.0f * scale;
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

        // Renders one skill's param widgets, generic over where the values
        // come from and where edits are written back to -- reused for both
        // a top-level task and a task nested inside a run_until group.
        auto draw_param_editor = [&](const nlohmann::json &params, const std::map<std::string, ParamSpec> &param_specs,
                                      const std::function<void(const std::string &, const nlohmann::json &)> &set_param) {
            for (const auto &[param_name, param_spec] : param_specs) {
                // No current skill uses polygon params (only the shelved
                // search_grid would); needs its own vertex-list editor
                // eventually.
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
                    ImGui::SetNextItemWidth(140.0f * scale);
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
                    ImGui::SetNextItemWidth(220.0f * scale);
                    if (ImGui::InputFloat3(param_name.c_str(), xyz)) {
                        set_param(param_name, nlohmann::json::array({xyz[0], xyz[1], xyz[2]}));
                    }
                } else if (param_spec.type == "int") {
                    int value = params.value(param_name, 0);
                    ImGui::SetNextItemWidth(140.0f * scale);
                    if (ImGui::InputInt(param_name.c_str(), &value)) {
                        set_param(param_name, value);
                    }
                } else {
                    // Typed input, not a slider -- easier to enter an exact
                    // value than drag one out precisely.
                    float value = static_cast<float>(params.value(param_name, 0.0));
                    ImGui::SetNextItemWidth(140.0f * scale);
                    if (ImGui::InputFloat(param_name.c_str(), &value)) {
                        set_param(param_name, static_cast<double>(value));
                    }
                }
                ImGui::PopID();
            }
        };

        // remove/move/wrap mutate sequence->children directly, so applying
        // one mid-loop would invalidate the indices/iterators the rest of
        // the loop relies on -- deferred until after the loop instead.
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

            if (!first_row) {
                const ImVec2 sep_pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(sep_pos.x, sep_pos.y - 5.0f * scale),
                    ImVec2(sep_pos.x + row_width, sep_pos.y - 5.0f * scale),
                    Color::panelBorder(theme), 1.0f * scale);
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
                // Distinct accent per wrapper kind so they read apart from
                // each other (and from a plain task's neutral badge).
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

                // Orange/blue/purple badge/text when otherwise-clean, to
                // read as visually distinct from a plain task at a glance.
                const ImU32 status_color = has_error   ? IM_COL32(255, 92, 92, 255)
                                          : has_warning ? IM_COL32(245, 200, 76, 255)
                                                        : accent;

                const ImVec2 row_start = ImGui::GetCursorScreenPos();
                const float badge_radius = 12.0f * scale;
                const float badge_diameter = badge_radius * 2.0f;
                const ImVec2 badge_center(row_start.x + badge_radius, row_start.y + badge_radius);

                const float button_size = ImGui::GetFrameHeight();
                const float buttons_w = button_size * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
                const float content_w = row_width - buttons_w - 8.0f * scale;

                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddCircle(badge_center, badge_radius, status_color, 0, 1.5f * scale);
                char index_label[16];
                std::snprintf(index_label, sizeof(index_label), "%d", display_index);
                const ImVec2 index_size = ImGui::CalcTextSize(index_label);
                draw_list->AddText(ImVec2(badge_center.x - index_size.x * 0.5f, badge_center.y - index_size.y * 0.5f),
                                   Color::white_black(theme), index_label);

                ImGui::SetCursorScreenPos(ImVec2(row_start.x + badge_diameter + 12.0f * scale, row_start.y));
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
                // Buttons align with just the title line, not the full
                // title+subtitle block -- otherwise they float at the
                // vertical center of both lines combined, which drifts away
                // from the title as the subtitle (task count + detail)
                // grows longer than a task's usually does.
                const float button_align_h = std::max(badge_diameter, title_line_h);

                const bool content_hovered = ImGui::IsMouseHoveringRect(
                    row_start, ImVec2(row_start.x + content_w, row_start.y + row_height));
                if (content_hovered) {
                    draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                             IM_COL32(255, 255, 255, 15), 6.0f * scale);
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
                ImGui::Dummy(ImVec2(row_width, row_height + 10.0f * scale));

                if (expanded_task_index_ == static_cast<int>(i)) {
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::Indent(badge_diameter + 12.0f * scale);
                    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));

                    if (run_until) {
                    // BeginCombo's text-preview area (FrameBg*) and its
                    // arrow box (Button*) are separate color slots -- easy
                    // to theme the former and still get a stock-colored
                    // arrow, same class of bug the vehicle dropdown had.
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

                        ImGui::SetNextItemWidth(140.0f * scale);
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
                        ImGui::SetNextItemWidth(70.0f * scale);
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
                            ImGui::SetNextItemWidth(90.0f * scale);
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
                        ImGui::SetNextItemWidth(90.0f * scale);
                        // step=0 disables the +/- buttons -- typing an
                        // exact value directly, not incrementing, matches
                        // this file's preference elsewhere (task params use
                        // plain InputFloat/InputInt over sliders too).
                        if (ImGui::InputInt("Count", &count, 0, 0)) {
                            planner_.set_repeat_count(i, count);
                        }
                        ImGui::SameLine();
                    } else {
                        int max_attempts = retry->max_attempts;
                        ImGui::SetNextItemWidth(90.0f * scale);
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
                    ImGui::Indent(16.0f * scale);

                    if (inner_child && inner_child->kind() == NodeKind::Sequence) {
                        const auto &inner = static_cast<const SequenceNode &>(*inner_child);
                        // Same deferred-mutation reasoning as the top-level
                        // loop: removing/reordering mid-iteration would
                        // invalidate inner.children out from under the loop
                        // still walking it.
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
                            const float nested_content_w = nested_row_width - nested_buttons_w - 8.0f * scale;

                            ImGui::BeginGroup();
                            ImGui::PushStyleColor(ImGuiCol_Text, nested_color);
                            ImGui::Text("%d. %s", static_cast<int>(t) + 1, ToUpper(nested_task.skill).c_str());
                            ImGui::PopStyleColor();
                            ImGui::PushStyleColor(ImGuiCol_Text, Color::dwhite_lblack(theme));
                            ImGui::TextUnformatted(TaskSubtitle(nested_task).c_str());
                            ImGui::PopStyleColor();
                            ImGui::EndGroup();
                            const float nested_row_height = ImGui::GetItemRectSize().y;

                            // Whole content block (both lines), not just
                            // the skill-name line, is the click target --
                            // matches the top-level row's own click area.
                            const bool nested_hovered = nested_has_params && ImGui::IsMouseHoveringRect(
                                nested_row_start, ImVec2(nested_row_start.x + nested_content_w, nested_row_start.y + nested_row_height));
                            if (nested_hovered) {
                                ImGui::GetWindowDrawList()->AddRectFilled(
                                    nested_row_start, ImVec2(nested_row_start.x + nested_content_w, nested_row_start.y + nested_row_height),
                                    IM_COL32(255, 255, 255, 15), 4.0f * scale);
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
                                ImGui::Indent(12.0f * scale);
                                draw_param_editor(nested_task.params, nested_spec->params,
                                    [&](const std::string &param_name, const nlohmann::json &value) {
                                        planner_.set_group_task_param(i, t, param_name, value);
                                    });
                                ImGui::Unindent(12.0f * scale);
                                ImGui::PopStyleColor(5);
                            }
                            ImGui::PopID();
                            ImGui::Spacing();
                        }

                        // Removing the last task in a group dissolves the
                        // whole group (planner_'s job), which would make
                        // expanded_task_index_ point at whatever now
                        // occupies this slot -- clear both indices rather
                        // than risk that.
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

                    ImGui::Unindent(16.0f * scale);
                    ImGui::Unindent(badge_diameter + 12.0f * scale);
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
            const bool has_editable_params = spec && !spec->params.empty();
            const bool is_selected = selected_task_indices_.count(i) > 0;

            const ImU32 status_color = has_error   ? IM_COL32(255, 92, 92, 255)
                                      : has_warning ? IM_COL32(245, 200, 76, 255)
                                                    : Color::panelBorder(theme);

            const ImVec2 row_start = ImGui::GetCursorScreenPos();
            const float badge_radius = 12.0f * scale;
            const float badge_diameter = badge_radius * 2.0f;
            const ImVec2 badge_center(row_start.x + badge_radius, row_start.y + badge_radius);

            // Hoisted so the hover/click check below can exclude the button
            // strip -- otherwise clicking a button would also register as a
            // click on the row content underneath it.
            const float button_size = ImGui::GetFrameHeight();
            const float buttons_w = button_size * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
            const float content_w = row_width - buttons_w - 8.0f * scale;

            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCircle(badge_center, badge_radius, status_color, 0, 1.5f * scale);
            char index_label[16];
            std::snprintf(index_label, sizeof(index_label), "%d", display_index);
            const ImVec2 index_size = ImGui::CalcTextSize(index_label);
            draw_list->AddText(ImVec2(badge_center.x - index_size.x * 0.5f, badge_center.y - index_size.y * 0.5f),
                               Color::white_black(theme), index_label);

            ImGui::SetCursorScreenPos(ImVec2(row_start.x + badge_diameter + 12.0f * scale, row_start.y));
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

            // Manual rect check rather than ImGui's item-hover system,
            // since a "row" here is a badge + text group + buttons, not one
            // ImGui item. Scoped to content_w so it doesn't also fire when
            // clicking the up/down/remove buttons on the same row.
            const bool content_hovered = ImGui::IsMouseHoveringRect(
                row_start, ImVec2(row_start.x + content_w, row_start.y + row_height));
            if (is_selected) {
                draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                         IM_COL32(255, 130, 30, 35), 6.0f * scale);
            } else if (content_hovered) {
                draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                         IM_COL32(255, 255, 255, 15), 6.0f * scale);
            }
            if (content_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (ImGui::GetIO().KeyCtrl) {
                    if (is_selected) { selected_task_indices_.erase(i); }
                    else { selected_task_indices_.insert(i); }
                } else if (has_editable_params) {
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
            // Always white -- this button's background is a fixed dark red
            // regardless of theme, so theme-toggled text goes low-contrast
            // (black) in light theme.
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
            // Uppercase -- lowercase "x" has no ascender/descender, so
            // ImGui centers it within the *font's* full line-height box
            // rather than its own visible ink, reading as slightly high.
            if (ImGui::Button("X", ImVec2(button_size, button_size))) { remove_index = static_cast<int>(i); }
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
            ImGui::PopID();

            // Dummy() reserves the row's space as a real item -- a manual
            // SetCursorScreenPos alone leaves ImGui unable to tell how far
            // the window's content extends past the last row.
            ImGui::SetCursorScreenPos(row_start);
            ImGui::Dummy(ImVec2(row_width, row_height + 10.0f * scale));

            // Expanded params render inline, in the normal item flow, so
            // they push subsequent rows down like any other block rather
            // than floating over them. PushID(i) keeps widget IDs unique
            // per row -- without it, two rows with a same-named param
            // (e.g. two goto tasks' "yaw") would share ImGui's ID and
            // fight over edit state.
            if (expanded_task_index_ == static_cast<int>(i)) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::Indent(badge_diameter + 12.0f * scale);
                ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::panelBorder(theme));
                ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 130, 30, 255));

                // spec is guaranteed non-null with non-empty params here --
                // has_editable_params gated the click that set
                // expanded_task_index_ to this row in the first place.
                draw_param_editor(task.params, spec->params,
                    [&](const std::string &param_name, const nlohmann::json &value) {
                        planner_.set_task_param(i, param_name, value);
                    });

                ImGui::PopStyleColor(5);
                ImGui::Unindent(badge_diameter + 12.0f * scale);
                ImGui::Spacing();
                ImGui::PopID();
            }
        }

        // Keep expanded_task_index_ pointing at the same row after a
        // reorder/removal/grouping shifts indices -- otherwise the wrong
        // row (or an out-of-range one) would appear expanded next frame.
        // Selection is simpler to just drop than to remap through an
        // arbitrary structural change.
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

    // Footer: always visible below the scrolling rows, not part of the
    // scrollable content.
    ImGui::PushStyleColor(ImGuiCol_Separator, Color::panelBorder(theme));
    ImGui::Separator();
    ImGui::PopStyleColor();

    // Centered within whatever space is left below the divider down to the
    // panel's bottom edge -- footer_h above reserves generous room, and
    // just flowing straight after Separator() left the buttons hugging its
    // top with dead air beneath instead of even margin above and below.
    const float button_h = ImGui::GetFontSize() + 20.0f * scale;  // matches the FramePadding pushed below
    const float band_remaining = ImGui::GetContentRegionAvail().y;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, (band_remaining - button_h) * 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f * scale, 10.0f * scale));
    PushThemedButtonStyle(theme, false);
    if (ImGui::Button("Save")) {
        save_load_dialog_.Open(SaveLoadDialog::Mode::Save, PlansDirectory(), ".json",
            [this](const std::string &path) {
                planner_.save(path);
                current_plan_name_ = std::filesystem::path(path).stem().string();
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
                current_plan_name_ = std::filesystem::path(path).stem().string();
            });
    }
    PopThemedButtonStyle();

    // Right-aligned on the same row as Save/Load, opposite side.
    ImGui::SameLine();
    const float clear_w = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - clear_w);
    // Always white, not theme-toggled -- this button's background is a
    // fixed dark red regardless of light/dark theme, so black text (what
    // white_black(theme) gives in light theme) is low-contrast.
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
    if (ImGui::Button("Clear")) {
        ImGui::OpenPopup("ConfirmClearPopup");
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();

    // NoTitleBar: BeginPopupModal would otherwise render its name argument
    // ("ConfirmClearPopup", an internal ID) as a literal title-bar caption.
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    // Centered on the app window -- this is a blocking confirmation with no
    // particular anchor point, unlike a row-relative control.
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("ConfirmClearPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("Clear the entire flight plan?");
        ImGui::Spacing();
        // Always white -- overrides the ambient theme-toggled Text color
        // pushed above, since this button's red background stays fixed
        // regardless of theme.
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
        if (ImGui::Button("Yes, clear it")) {
            planner_.clear();
            expanded_task_index_ = -1;
            expanded_group_task_index_ = -1;
            selected_task_indices_.clear();
            current_plan_name_.clear();
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
