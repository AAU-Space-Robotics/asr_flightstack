#pragma once

// ImGui rendering for the "Mission Planner" tab. Reads/mutates plan state
// via a Planner reference; no ROS or plan-model logic belongs in this file.

#include <set>
#include <string>
#include <utility>

#include "planner.h"
#include "save_load_dialog.h"

class PlannerPanel {
public:
    explicit PlannerPanel(Planner &planner);

    void Draw(float scale, bool theme, float window_height);

    // (top_level_index, nested_index) of the currently expanded row, mirroring
    // expanded_task_index_/expanded_group_task_index_ -- nested_index is -1
    // unless that row is actually a group. (-1, -1) if nothing is expanded.
    // Used to mirror the list's selection onto the map overlay's markers.
    std::pair<int, int> highlighted_task() const;

    // Expands the given top-level row (and, if it's a group, the given
    // nested task within it) and scrolls it into view -- mirrors a map
    // marker click back onto the list.
    void SelectTask(size_t top_level_index, int nested_index);

private:
    Planner &planner_;

    SaveLoadDialog save_load_dialog_;

    // Name of the file the plan was last saved to or loaded from -- empty
    // until either happens, or after Clear.
    std::string current_plan_name_;

    // Index of the top-level row (task or run_until group) whose block is
    // expanded inline below it, or -1 if none. Only one at a time.
    int expanded_task_index_ = -1;

    // Which nested task within the currently-expanded group has its own
    // param editor open, or -1 if none / the expanded row isn't a group.
    int expanded_group_task_index_ = -1;

    // Ctrl+click toggles membership; only plain (ungrouped) task rows are
    // selectable, for wrapping into a run_until group.
    std::set<size_t> selected_task_indices_;

    // One-shot: set by SelectTask(), consumed (and cleared) by the next
    // DrawTaskList() call once it scrolls the target row into view.
    bool scroll_to_expanded_ = false;

    // Vehicle dropdown + skill palette, one panel.
    void DrawVehicleAndPalette(float scale, bool theme);

    // The numbered task list, with per-row reorder/remove/param-edit.
    void DrawTaskList(float scale, bool theme, float window_height);
};
