#pragma once

#include <imgui.h>

#include "planner/save_load_dialog.h"

class Planner;

// Bottom-centered bar on the flight page: Load/Clear/Upload/Execute/Abort -- flight-ops only, no task editing.
class MissionControlBar {
public:
    // Returns the bar's own top Y, so callers can keep other content (e.g. a foreground-drawn map overlay) above it.
    float Draw(Planner &planner, float scale, bool theme,
               float map_x, float map_y, float map_w, float map_h);

    // Same top-Y as Draw() would return, but without opening the window -- safe to call before the map.
    float PanelTopY(float scale, float map_y, float map_h) const;

private:
    SaveLoadDialog save_load_dialog_;
};
