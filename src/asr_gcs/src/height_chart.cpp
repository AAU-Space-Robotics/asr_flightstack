#include "height_chart.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <implot.h>

#include "asr_mission/plan.h"
#include "info_panels.h"

using namespace asr_mission;

namespace {

// Walks the plan in execution order, appending one altitude per task.
// `carry` is the "current altitude" the vehicle would be at -- tasks that
// don't set it (spin/rth/rtl/unmodeled skills) just hold it flat rather
// than dropping to 0 or being skipped, so the line reads as continuous.
void CollectAltitudes(const PlanNode &node, std::optional<double> &carry, std::vector<double> &altitudes) {
    switch (node.kind()) {
        case NodeKind::Task: {
            const auto &task = static_cast<const TaskNode &>(node);
            if (task.skill == "takeoff") {
                carry = task.params.value("alt", carry.value_or(0.0));
            } else if (task.skill == "goto") {
                const auto pos = task.params.value("pos", std::vector<double>{});
                if (pos.size() == 3) { carry = pos[2]; }
            } else if (task.skill == "land") {
                carry = 0.0;
            }
            altitudes.push_back(carry.value_or(0.0));
            break;
        }
        case NodeKind::Sequence:
            for (const auto &child : static_cast<const SequenceNode &>(node).children) {
                CollectAltitudes(*child, carry, altitudes);
            }
            break;
        case NodeKind::RunUntil: {
            const auto &run_until = static_cast<const RunUntilNode &>(node);
            if (run_until.child) { CollectAltitudes(*run_until.child, carry, altitudes); }
            break;
        }
        case NodeKind::Repeat: {
            // Counted once, not `count` times -- this is a plan overview,
            // not a simulation of the full executed timeline.
            const auto &repeat = static_cast<const RepeatNode &>(node);
            if (repeat.child) { CollectAltitudes(*repeat.child, carry, altitudes); }
            break;
        }
        case NodeKind::Retry: {
            const auto &retry = static_cast<const RetryNode &>(node);
            if (retry.child) { CollectAltitudes(*retry.child, carry, altitudes); }
            break;
        }
    }
}

} // namespace

void DrawHeightChart(const Plan &plan, ImVec2 pos, ImVec2 size, float scale, bool theme)
{
    BeginFixedPanel("HeightChartPanel", pos, size, scale, theme, 0, ImVec2(8, 8));

    std::vector<double> altitudes;
    if (plan.root) {
        std::optional<double> carry = 0.0;
        CollectAltitudes(*plan.root, carry, altitudes);
    }

    ImPlot::PushStyleColor(ImPlotCol_FrameBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, Color::panelColor(theme));
    ImPlot::PushStyleColor(ImPlotCol_PlotBorder, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisText, Color::white_black(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisGrid, Color::panelBorder(theme));
    ImPlot::PushStyleColor(ImPlotCol_AxisTick, Color::panelBorder(theme));

    // Y is inverted -- this app's own plan-authoring convention has
    // pos.z/alt positive-up (see thyra's skills.yaml), but the vehicle's
    // native NED frame has "up" as negative, which is the more familiar
    // reading here.
    if (ImPlot::BeginPlot("##HeightChart", ImVec2(-1, -1), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxis(ImAxis_X1, "Task");
        ImPlot::SetupAxis(ImAxis_Y1, "Altitude (m)", ImPlotAxisFlags_Invert);

        if (altitudes.empty()) {
            // Still shows the plot frame/axes rather than a blank "no
            // data" placeholder, so the panel doesn't look broken/empty.
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 0, ImPlotCond_Always);
        } else {
            // Explicit limits (not ImPlotAxisFlags_AutoFit) so padding is
            // guaranteed even when the profile plateaus -- AutoFit hugs
            // the data exactly, which puts a flat top/bottom run right up
            // against the plot border where it's hard to see. At least 1m
            // of headroom, or 10% of the range for a taller profile so the
            // margin still looks proportionate.
            const double min_alt = *std::min_element(altitudes.begin(), altitudes.end());
            const double max_alt = *std::max_element(altitudes.begin(), altitudes.end());
            const double padding = std::max(1.0, (max_alt - min_alt) * 0.1);
            ImPlot::SetupAxisLimits(ImAxis_Y1, min_alt - padding, max_alt + padding, ImPlotCond_Always);

            std::vector<double> xs(altitudes.size());
            // One tick per task, explicitly -- ImPlot's automatic tick
            // spacing doesn't know "Task" is a discrete, integer-only axis
            // and will happily subdivide it (e.g. showing "2.2").
            std::vector<std::string> tick_label_strings(altitudes.size());
            std::vector<const char *> tick_labels(altitudes.size());
            for (size_t k = 0; k < xs.size(); ++k) {
                xs[k] = static_cast<double>(k + 1);
                tick_label_strings[k] = std::to_string(k + 1);
                tick_labels[k] = tick_label_strings[k].c_str();
            }
            ImPlot::SetupAxisTicks(ImAxis_X1, xs.data(), static_cast<int>(xs.size()), tick_labels.data());
            ImPlot::SetupAxisLimits(ImAxis_X1, 1, static_cast<double>(altitudes.size()), ImPlotCond_Always);

            ImPlot::PlotLine("Altitude", xs.data(), altitudes.data(), static_cast<int>(altitudes.size()),
                             {ImPlotProp_LineColor, IM_COL32(255, 130, 30, 255)});
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor(6);

    EndFixedPanel();
}
