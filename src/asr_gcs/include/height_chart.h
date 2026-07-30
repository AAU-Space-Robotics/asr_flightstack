#pragma once

#include <imgui.h>

namespace asr_mission { class Plan; }

// Altitude profile of `plan`, drawn as its own themed panel (an ImPlot
// strip) at `pos`/`size` -- one point per task with an explicit altitude
// (takeoff.alt, goto.pos[2], land -> 0), holding flat across tasks that
// don't have one (spin/rth/rtl) so the line reads as continuous. Includes
// tasks nested inside run_until/repeat/retry groups, each counted once
// regardless of how many times a repeat would actually run it.
void DrawHeightChart(const asr_mission::Plan &plan, ImVec2 pos, ImVec2 size, float scale, bool theme);
