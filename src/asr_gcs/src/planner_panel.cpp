#include "planner_panel.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
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

    // --- Header: title + item-count pill -----------------------------
    // header_top_y/title_h capture the title line's own span so every other
    // element on this row (name, Clear, pill) can center itself against
    // that span, rather than each other's differing heights.
    const float header_top_y = ImGui::GetCursorPosY();
    ImGui::PushFont(winInit.getFont(24));  // bold 24
    const float title_h = ImGui::GetFontSize();
    ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
    ImGui::TextUnformatted("FLIGHT PLAN");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(header_top_y + (title_h - ImGui::GetFontSize()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, Color::dwhite_lblack(theme));
    ImGui::Text("- %s", current_plan_name_.empty() ? "untitled" : current_plan_name_.c_str());
    ImGui::PopStyleColor();
    const float header_center_y = header_top_y + title_h * 0.5f;

    {
        char count_text[24];
        std::snprintf(count_text, sizeof(count_text), "%zu item%s", count, count == 1 ? "" : "s");
        const ImVec2 text_size = ImGui::CalcTextSize(count_text);
        const float pill_w = text_size.x + 20.0f * scale;
        const float pill_h = text_size.y + 6.0f * scale;

        const ImVec2 clear_label_size = ImGui::CalcTextSize("Clear");
        const float clear_w = clear_label_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float clear_h = clear_label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f;
        const float pill_x = ImGui::GetWindowContentRegionMax().x - pill_w;
        const float clear_x = pill_x - 8.0f * scale - clear_w;

        ImGui::SetCursorPos(ImVec2(clear_x, header_center_y - clear_h * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
        if (ImGui::Button("Clear")) {
            ImGui::OpenPopup("ConfirmClearPopup");
        }
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();

        // NoTitleBar: BeginPopupModal would otherwise render its name
        // argument ("ConfirmClearPopup", an internal ID) as a literal
        // title-bar caption.
        ImGui::PushStyleColor(ImGuiCol_PopupBg, Color::panelColor(theme));
        ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
        ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
        // Centered on the app window -- this is a blocking confirmation
        // with no particular anchor point, unlike a row-relative control.
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("ConfirmClearPopup", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
            ImGui::Text("Clear the entire flight plan?");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
            if (ImGui::Button("Yes, clear it")) {
                planner_.clear();
                expanded_task_index_ = -1;
                current_plan_name_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            PushThemedButtonStyle(theme, false);
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            PopThemedButtonStyle();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(ImVec2(pill_x, header_center_y - pill_h * 0.5f));
        const ImVec2 screen_pos = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + pill_w, screen_pos.y + pill_h),
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

    // --- Rows: numbered badge + skill name/subtitle -------------------
    if (!sequence || sequence->children.empty()) {
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, Color::dwhite_lblack(theme));
        ImGui::TextDisabled("No tasks yet -- pick a skill above to get started.");
        ImGui::PopStyleColor();
    } else {
        const float row_width = ImGui::GetContentRegionAvail().x;
        const std::vector<Issue> issues = planner_.local_issues();
        const VehicleCapabilities *task_capabilities = planner_.selected_capabilities();

        // remove/move mutate sequence->children directly, so applying one
        // mid-loop would invalidate the indices/iterators the rest of the
        // loop relies on -- deferred until after the loop instead.
        int remove_index = -1;
        int move_up_index = -1;
        int move_down_index = -1;
        bool first_row = true;

        for (size_t i = 0; i < sequence->children.size(); ++i) {
            const auto &child = sequence->children[i];
            if (child->kind() != NodeKind::Task) { continue; }
            const auto &task = static_cast<const TaskNode &>(*child);
            const int display_index = static_cast<int>(i) + 1;
            const std::string node_path = "root.children[" + std::to_string(i) + "]";

            const SkillSpec *spec = nullptr;
            if (task_capabilities) {
                auto skill_it = task_capabilities->skills.find(task.skill);
                if (skill_it != task_capabilities->skills.end()) { spec = &skill_it->second; }
            }
            const bool has_editable_params = spec && !spec->params.empty();

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
            const bool content_hovered = has_editable_params && ImGui::IsMouseHoveringRect(
                row_start, ImVec2(row_start.x + content_w, row_start.y + row_height));
            if (content_hovered) {
                draw_list->AddRectFilled(row_start, ImVec2(row_start.x + content_w, row_start.y + row_height),
                                         IM_COL32(255, 255, 255, 15), 6.0f * scale);
            }
            if (content_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                expanded_task_index_ = (expanded_task_index_ == static_cast<int>(i)) ? -1 : static_cast<int>(i);
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
            ImGui::PushStyleColor(ImGuiCol_Text, Color::white_black(theme));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(140, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(200, 70, 70, 255));
            if (ImGui::Button("x", ImVec2(button_size, button_size))) { remove_index = static_cast<int>(i); }
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
                for (const auto &[param_name, param_spec] : spec->params) {
                    // No current skill uses polygon params (only the
                    // shelved search_grid would); needs its own
                    // vertex-list editor eventually.
                    if (param_spec.type == "polygon") {
                        ImGui::TextDisabled("%s: editing not supported yet", param_name.c_str());
                        continue;
                    }

                    ImGui::PushID(param_name.c_str());
                    if (param_spec.type == "bool") {
                        bool value = task.params.value(param_name, false);
                        if (ImGui::Checkbox(param_name.c_str(), &value)) {
                            planner_.set_task_param(i, param_name, value);
                        }
                    } else if (param_spec.type == "string") {
                        const std::string current = task.params.value(param_name, std::string());
                        char buffer[128];
                        std::snprintf(buffer, sizeof(buffer), "%s", current.c_str());
                        ImGui::SetNextItemWidth(140.0f * scale);
                        if (ImGui::InputText(param_name.c_str(), buffer, sizeof(buffer))) {
                            planner_.set_task_param(i, param_name, std::string(buffer));
                        }
                    } else if (param_spec.type == "point") {
                        const auto current = task.params.value(param_name, std::vector<double>{0.0, 0.0, 0.0});
                        float xyz[3] = {
                            static_cast<float>(current.size() > 0 ? current[0] : 0.0),
                            static_cast<float>(current.size() > 1 ? current[1] : 0.0),
                            static_cast<float>(current.size() > 2 ? current[2] : 0.0),
                        };
                        ImGui::SetNextItemWidth(220.0f * scale);
                        if (ImGui::InputFloat3(param_name.c_str(), xyz)) {
                            planner_.set_task_param(i, param_name,
                                nlohmann::json::array({xyz[0], xyz[1], xyz[2]}));
                        }
                    } else {
                        // Typed input, not a slider -- easier to enter
                        // an exact value than drag one out precisely.
                        float value = static_cast<float>(task.params.value(param_name, 0.0));
                        ImGui::SetNextItemWidth(140.0f * scale);
                        const bool changed = ImGui::InputFloat(param_name.c_str(), &value);
                        if (changed) {
                            planner_.set_task_param(i, param_name,
                                param_spec.type == "int" ? nlohmann::json(static_cast<int>(value))
                                                         : nlohmann::json(static_cast<double>(value)));
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::PopStyleColor(5);
                ImGui::Unindent(badge_diameter + 12.0f * scale);
                ImGui::Spacing();
                ImGui::PopID();
            }
        }

        // Keep expanded_task_index_ pointing at the same task after a
        // reorder/removal shifts indices -- otherwise the wrong row (or an
        // out-of-range one) would appear expanded next frame.
        if (remove_index >= 0) {
            planner_.remove_task(static_cast<size_t>(remove_index));
            if (remove_index == expanded_task_index_) {
                expanded_task_index_ = -1;
            } else if (remove_index < expanded_task_index_) {
                --expanded_task_index_;
            }
        } else if (move_up_index >= 0) {
            planner_.move_task_up(static_cast<size_t>(move_up_index));
            if (expanded_task_index_ == move_up_index) {
                expanded_task_index_ = move_up_index - 1;
            } else if (expanded_task_index_ == move_up_index - 1) {
                expanded_task_index_ = move_up_index;
            }
        } else if (move_down_index >= 0) {
            planner_.move_task_down(static_cast<size_t>(move_down_index));
            if (expanded_task_index_ == move_down_index) {
                expanded_task_index_ = move_down_index + 1;
            } else if (expanded_task_index_ == move_down_index + 1) {
                expanded_task_index_ = move_down_index;
            }
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
                current_plan_name_ = std::filesystem::path(path).stem().string();
            });
    }
    PopThemedButtonStyle();
    ImGui::PopStyleVar();

    save_load_dialog_.Draw(scale, theme);

    EndFixedPanel();
    ImGui::PopStyleColor(4);
}
