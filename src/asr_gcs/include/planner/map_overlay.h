#pragma once

#include <imgui.h>
#include "app_window.h"

namespace asr_mission { class Plan; }
class Location;

// Which top-level task row (and nested task, if it's a group) a marker click resolved to this frame, if any.
struct MapTaskClick {
    bool clicked = false;
    size_t top_level_index = 0;
    int nested_index = -1;
};

// Draws the plan's spatial route (numbered markers + line) on top of an already-rendered MapWidget -- `widget_pos`/`width`/`height` must be exactly what that call was given, since marker positions are computed relative to its own center. `visible_h` (<= height) only bounds what's actually drawn/clickable, e.g. to stay clear of other UI overlapping the same widget. Returns which marker (if any) was clicked this frame.
MapTaskClick DrawPlanRouteOverlay(Location &location, const asr_mission::Plan &plan,
                                  double home_lat, double home_lon,
                                  double center_lat, double center_lon, int zoom,
                                  ImVec2 widget_pos, float width, float height, float visible_h, const UiScale& scale,
                                  int highlighted_top_level, int highlighted_nested);

// Draws a single marker for the vehicle's live GPS position -- same widget_pos/width/height/center convention as DrawPlanRouteOverlay, so it lines up on a view centered on something other than the vehicle itself (e.g. a fixed origin).
void DrawUavPositionMarker(Location &location, double uav_lat, double uav_lon,
                           double center_lat, double center_lon, int zoom,
                           ImVec2 widget_pos, float width, float height, float visible_h, const UiScale& scale);
