#pragma once

// ImGui rendering for the "Mission Planner" tab. Reads/mutates plan state
// via a Planner reference; no ROS or plan-model logic belongs in this file.

#include <string>

#include "planner.h"
#include "save_load_dialog.h"

class PlannerPanel {
public:
    explicit PlannerPanel(Planner &planner);

    void Draw(float scale, bool theme);

private:
    Planner &planner_;

    SaveLoadDialog save_load_dialog_;

    // Name of the file the plan was last saved to or loaded from -- empty
    // until either happens, or after Clear.
    std::string current_plan_name_;

    // Index of the task whose param block is expanded inline below its row,
    // or -1 if none. Only one at a time (accordion-style).
    int expanded_task_index_ = -1;

    // Vehicle dropdown + skill palette, one panel.
    void DrawVehicleAndPalette(float scale, bool theme);

    // The numbered task list, with per-row reorder/remove/param-edit.
    void DrawTaskList(float scale, bool theme);

    // Still to come: DrawMapOverlay (spatial tasks on the map) and
    // DrawHeightChart (ImPlot strip, altitude per spatial task).
};
