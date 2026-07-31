#pragma once

#include <imgui.h>

namespace asr_mission { class Plan; }

// Altitude profile of `plan`, drawn as its own themed panel (an ImPlot
// strip) at `pos`/`size` -- one point per task with an explicit altitude
// (takeoff.alt, goto.pos[2], land -> 0), holding flat across tasks that
// don't have one (spin/rth/rtl). Includes tasks nested inside run_until/
// repeat/retry groups; a repeat's span is drawn once and marked with a
// shaded band + "repeat Nx" label rather than plotted count times.
void DrawHeightChart(const asr_mission::Plan &plan, ImVec2 pos, ImVec2 size, float scale, bool theme);
