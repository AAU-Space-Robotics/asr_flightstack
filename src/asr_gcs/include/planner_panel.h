#pragma once

// ImGui rendering for the "Mission Planner" tab. Reads/mutates plan state
// via a Planner reference; no ROS or plan-model logic belongs in this file.

#include "planner.h"

class PlannerPanel {
public:
    explicit PlannerPanel(Planner &planner);

    void Draw(float scale, bool theme);

private:
    Planner &planner_;

    // Vehicle dropdown + skill palette, one panel.
    void DrawVehicleAndPalette(float scale, bool theme);

    // The numbered task list, with per-row reorder/remove/param-edit.
    void DrawTaskList(float scale, bool theme);

    // Still to come: DrawMapOverlay (spatial tasks on the map) and
    // DrawHeightChart (ImPlot strip, altitude per spatial task).
};
