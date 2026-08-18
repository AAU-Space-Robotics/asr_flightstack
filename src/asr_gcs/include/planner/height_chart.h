#pragma once

#include <imgui.h>

namespace asr_mission { class Plan; }

// Altitude profile of `plan` as its own themed ImPlot panel; a repeat's span gets a shaded band, not expanded points.
void DrawHeightChart(const asr_mission::Plan &plan, ImVec2 pos, ImVec2 size, float scale, bool theme);
