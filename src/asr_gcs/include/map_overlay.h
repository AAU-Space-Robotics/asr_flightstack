#pragma once

#include <imgui.h>

namespace asr_mission { class Plan; }
class Location;

// Which top-level task row (and, if it's a group, which nested task within
// it) a marker click resolved to this frame, if any.
struct MapTaskClick {
    bool clicked = false;
    size_t top_level_index = 0;
    int nested_index = -1;
};

// Draws the plan's spatial route on top of an already-rendered MapWidget:
// one numbered marker per task (same flattened execution order the height
// chart uses), connected by a line. Each task's local NED offset (carried
// forward from its last `goto`, starting at home) is projected through
// `home_lat`/`home_lon`, then placed on screen via `location`'s own
// `center_lat`/`center_lon`/`zoom` -- `widget_pos` must be exactly what that
// MapWidget call returned, so markers land on the tiles they belong on.
//
// `highlighted_top_level`/`highlighted_nested` (-1 for neither) mark which
// task's marker(s) to draw emphasized, mirroring the task list's own
// expanded-row selection. Returns which marker (if any) was clicked this
// frame, so the caller can mirror it back onto the task list.
MapTaskClick DrawPlanRouteOverlay(Location &location, const asr_mission::Plan &plan,
                                  double home_lat, double home_lon,
                                  double center_lat, double center_lon, int zoom,
                                  ImVec2 widget_pos, float width, float height, float scale,
                                  int highlighted_top_level, int highlighted_nested);
