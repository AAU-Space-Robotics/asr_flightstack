#include "height_chart.h"

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include <implot.h>

#include "asr_mission/plan.h"
#include "info_panels.h"

using namespace asr_mission;

namespace {

// A repeat node's span within the flattened `altitudes` list -- lets the
// chart mark it as one bounded region instead of expanding its points
// `count` times.
struct RepeatBand {
    size_t start_index;
    size_t end_index;
    int count;
};

// Walks the plan in execution order, appending one altitude per task.
// `carry` holds flat across tasks that don't set it (spin/rth/unmodeled
// skills) so the line reads as continuous.
void CollectAltitudes(const PlanNode &node, std::optional<double> &carry, std::vector<double> &altitudes,
                       std::vector<RepeatBand> &bands) {
    switch (node.kind()) {
        case NodeKind::Task: {
            const auto &task = static_cast<const TaskNode &>(node);
            if (task.skill == "takeoff") {
                carry = task.params.value("alt", carry.value_or(0.0));
            } else if (task.skill == "goto") {
                const auto pos = task.params.value("pos", std::vector<double>{});
                if (pos.size() == 3) { carry = pos[2]; }
            } else if (task.skill == "land" || task.skill == "rtl") {
                // rtl flies home, then lands there -- same terminal altitude as land.
                carry = 0.0;
            }
            altitudes.push_back(carry.value_or(0.0));
            break;
        }
        case NodeKind::Sequence:
            for (const auto &child : static_cast<const SequenceNode &>(node).children) {
                CollectAltitudes(*child, carry, altitudes, bands);
            }
            break;
        case NodeKind::RunUntil: {
            const auto &run_until = static_cast<const RunUntilNode &>(node);
            if (run_until.child) { CollectAltitudes(*run_until.child, carry, altitudes, bands); }
            break;
        }
        case NodeKind::Repeat: {
            const auto &repeat = static_cast<const RepeatNode &>(node);
            if (repeat.child) {
                const size_t start = altitudes.size();
                CollectAltitudes(*repeat.child, carry, altitudes, bands);
                if (altitudes.size() > start) {
                    bands.push_back({start, altitudes.size() - 1, repeat.count});
                }
            }
            break;
        }
        case NodeKind::Retry: {
            const auto &retry = static_cast<const RetryNode &>(node);
            if (retry.child) { CollectAltitudes(*retry.child, carry, altitudes, bands); }
            break;
        }
    }
}

} // namespace

void DrawHeightChart(const Plan &plan, ImVec2 pos, ImVec2 size, float scale, bool theme)
{
    BeginFixedPanel("HeightChartPanel", pos, size, scale, theme, 0, ImVec2(8, 8));

    std::vector<double> altitudes;
    std::vector<RepeatBand> bands;
    if (plan.root) {
        std::optional<double> carry = 0.0;
        CollectAltitudes(*plan.root, carry, altitudes, bands);
    }

    ImPlot::PushStyleColor(ImPlotCol_FrameBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBorder, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisText, Color::white_black(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisGrid, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisTick, Color::panelBorder(theme));

    // Y is inverted to match the vehicle's native NED frame (up = negative),
    // even though plans themselves author alt/pos.z as positive-up.
    if (ImPlot::BeginPlot("##HeightChart", ImVec2(-1, -1), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxis(ImAxis_X1, "Task");
        ImPlot::SetupAxis(ImAxis_Y1, "Altitude (m)", ImPlotAxisFlags_Invert);

        if (altitudes.empty()) {
            // Still shows the plot frame/axes rather than a blank placeholder.
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 0, ImPlotCond_Always);
        } else {
            // Explicit limits, not ImPlotAxisFlags_AutoFit, so a plateau
            // still gets at least 1m (or 10%) of headroom instead of hugging the border.
            const double min_alt = *std::min_element(altitudes.begin(), altitudes.end());
            const double max_alt = *std::max_element(altitudes.begin(), altitudes.end());
            const double padding = std::max(1.0, (max_alt - min_alt) * 0.1);
            ImPlot::SetupAxisLimits(ImAxis_Y1, min_alt - padding, max_alt + padding, ImPlotCond_Always);

            std::vector<double> xs(altitudes.size());
            // One tick per task, explicit -- ImPlot's automatic spacing
            // would otherwise subdivide this discrete axis (e.g. "2.2").
            std::vector<std::string> tick_label_strings(altitudes.size());
            std::vector<const char *> tick_labels(altitudes.size());
            for (size_t k = 0; k < xs.size(); ++k) {
                xs[k] = static_cast<double>(k + 1);
                tick_label_strings[k] = std::to_string(k + 1);
                tick_labels[k] = tick_label_strings[k].c_str();
            }
            ImPlot::SetupAxisTicks(ImAxis_X1, xs.data(), static_cast<int>(xs.size()), tick_labels.data());
            ImPlot::SetupAxisLimits(ImAxis_X1, 1, static_cast<double>(altitudes.size()), ImPlotCond_Always);

            // Repeat bands drawn before the line so it renders on top.
            // Pixel-space top/bottom are resolved via min/max of the two
            // converted Y-limit points rather than assumed from the data's
            // own min/max, since ImPlotAxisFlags_Invert flips which one
            // lands at the top of the screen.
            if (!bands.empty()) {
                const ImPlotRect limits = ImPlot::GetPlotLimits();
                const float y_top = std::min(ImPlot::PlotToPixels(0, limits.Y.Min).y, ImPlot::PlotToPixels(0, limits.Y.Max).y);
                const float y_bottom = std::max(ImPlot::PlotToPixels(0, limits.Y.Min).y, ImPlot::PlotToPixels(0, limits.Y.Max).y);
                ImDrawList *plot_draw_list = ImPlot::GetPlotDrawList();
                ImPlot::PushPlotClipRect();
                for (const auto &band : bands) {
                    const float x_left = ImPlot::PlotToPixels(xs[band.start_index] - 0.5, 0).x;
                    const float x_right = ImPlot::PlotToPixels(xs[band.end_index] + 0.5, 0).x;
                    plot_draw_list->AddRectFilled(ImVec2(x_left, y_top), ImVec2(x_right, y_bottom),
                                                  IM_COL32(90, 150, 255, 45));

                    char label[32];
                    std::snprintf(label, sizeof(label), "repeat %dx", band.count);
                    const ImVec2 label_size = ImGui::CalcTextSize(label);
                    plot_draw_list->AddText(
                        ImVec2((x_left + x_right) * 0.5f - label_size.x * 0.5f, y_bottom - label_size.y - 4.0f * scale),
                        IM_COL32(90, 150, 255, 255), label);
                }
                ImPlot::PopPlotClipRect();
            }

            ImPlot::PlotLine("Altitude", xs.data(), altitudes.data(), static_cast<int>(altitudes.size()),
                             {ImPlotProp_LineColor, IM_COL32(255, 130, 30, 255)});
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor(6);

    EndFixedPanel();
}
