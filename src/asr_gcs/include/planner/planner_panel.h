#pragma once

// ImGui rendering for the "Mission Planner" tab; no ROS or plan-model logic belongs in this file.

#include <set>
#include <string>
#include <utility>

#include <imgui.h>

#include "planner/planner.h"
#include "planner/save_load_dialog.h"

class PlannerPanel {
public:
    explicit PlannerPanel(Planner &planner);

    void Draw(float scale, bool theme, float window_height);

    // (top_level_index, nested_index) of the currently expanded row, or (-1, -1) -- feeds the map overlay's markers.
    std::pair<int, int> highlighted_task() const;

    // Expands the given row (and scrolls it into view) -- mirrors a map marker click back onto the list.
    void SelectTask(size_t top_level_index, int nested_index);

private:
    Planner &planner_;

    SaveLoadDialog save_load_dialog_;

    // Index of the top-level row whose block is expanded inline below it, or -1 if none.
    int expanded_task_index_ = -1;

    // Which nested task within the currently-expanded group has its own param editor open, or -1.
    int expanded_group_task_index_ = -1;

    // Ctrl+click toggles membership; only plain task rows are selectable, for wrapping into a group.
    std::set<size_t> selected_task_indices_;

    // One-shot: set by SelectTask(), consumed by the next DrawTaskList() call.
    bool scroll_to_expanded_ = false;

    // Palette hover tooltip -- which skill button, when the hover started (ImGui::GetTime()), and that button's own bottom-right corner (the toast's anchor).
    std::string hovered_skill_;
    double hover_start_time_ = 0.0;
    ImVec2 hovered_button_max_;
    static constexpr double kHoverTooltipDelay = 1.5;

    // Vehicle dropdown + skill palette, one panel.
    void DrawVehicleAndPalette(float scale, bool theme);

    // Bottom-right toast with the hovered palette button's description, once hovered_skill_ has been held long enough.
    void DrawPaletteHoverToast(float scale, bool theme, const std::string &description);

    // The numbered task list, with per-row reorder/remove/param-edit.
    void DrawTaskList(float scale, bool theme, float window_height);
};
