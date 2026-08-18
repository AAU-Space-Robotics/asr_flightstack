#include "planner/map_overlay.h"

#include <cstdio>
#include <vector>

#include "asr_mission/plan.h"
#include "map.h"

using namespace asr_mission;

namespace {

struct RoutePoint {
    double north_m;
    double east_m;
    size_t top_level_index;
    int nested_index;  // -1 if this is a plain top-level task, not nested in a group
};

const PlanNode *WrapperChild(const PlanNode &node) {
    switch (node.kind()) {
        case NodeKind::RunUntil: return static_cast<const RunUntilNode &>(node).child.get();
        case NodeKind::Repeat:   return static_cast<const RepeatNode &>(node).child.get();
        case NodeKind::Retry:    return static_cast<const RetryNode &>(node).child.get();
        default: return nullptr;
    }
}

// Mirrors the task list's own two-level addressing instead of a generic tree walk, so markers can reference rows.
void CollectPositions(const Plan &plan, std::vector<RoutePoint> &points) {
    if (!plan.root || plan.root->kind() != NodeKind::Sequence) { return; }
    const auto &sequence = static_cast<const SequenceNode &>(*plan.root);

    RoutePoint carry{0.0, 0.0, 0, -1};
    auto apply_task = [&](const TaskNode &task) {
        if (task.skill == "goto") {
            const auto pos = task.params.value("pos", std::vector<double>{});
            if (pos.size() == 3) { carry.north_m = pos[0]; carry.east_m = pos[1]; }
        } else if (task.skill == "rth" || task.skill == "rtl") {
            // Both fly to (0, 0, current altitude) via asr_autopilot's flyToHome.
            carry.north_m = 0.0;
            carry.east_m = 0.0;
        }
    };

    for (size_t i = 0; i < sequence.children.size(); ++i) {
        const PlanNode *child = sequence.children[i].get();
        if (child->kind() == NodeKind::Task) {
            apply_task(static_cast<const TaskNode &>(*child));
            points.push_back({carry.north_m, carry.east_m, i, -1});
            continue;
        }

        const PlanNode *inner = WrapperChild(*child);
        if (!inner || inner->kind() != NodeKind::Sequence) { continue; }
        const auto &inner_seq = static_cast<const SequenceNode &>(*inner);
        for (size_t t = 0; t < inner_seq.children.size(); ++t) {
            if (inner_seq.children[t]->kind() != NodeKind::Task) { continue; }
            apply_task(static_cast<const TaskNode &>(*inner_seq.children[t]));
            points.push_back({carry.north_m, carry.east_m, i, static_cast<int>(t)});
        }
    }
}

} // namespace

MapTaskClick DrawPlanRouteOverlay(Location &location, const Plan &plan,
                                  double home_lat, double home_lon,
                                  double center_lat, double center_lon, int zoom,
                                  ImVec2 widget_pos, float width, float height, float visible_h, float scale,
                                  int highlighted_top_level, int highlighted_nested)
{
    MapTaskClick click;

    std::vector<RoutePoint> points;
    CollectPositions(plan, points);
    if (points.empty()) { return click; }

    std::vector<ImVec2> screen_points(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        double lat, lon;
        LocalOffsetToLatLon(home_lat, home_lon, points[i].north_m, points[i].east_m, lat, lon);
        screen_points[i] = location.latLonToScreenPos(lat, lon, center_lat, center_lon,
                                                       widget_pos, width, height, scale, zoom);
    }

    // Foreground, not GetWindowDrawList() -- MapWidget's tiles live on a child window's own draw list, which always composites on top of the parent regardless of call order.
    ImDrawList *draw_list = ImGui::GetForegroundDrawList();
    draw_list->PushClipRect(widget_pos, ImVec2(widget_pos.x + width, widget_pos.y + visible_h), true);

    if (screen_points.size() > 1) {
        draw_list->AddPolyline(screen_points.data(), static_cast<int>(screen_points.size()),
                               IM_COL32(255, 130, 30, 220), 2.0f * scale);
    }

    const ImVec2 widget_min = widget_pos;
    const ImVec2 widget_max(widget_pos.x + width, widget_pos.y + visible_h);
    const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    std::vector<bool> highlighted(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        highlighted[i] = highlighted_top_level >= 0 &&
            points[i].top_level_index == static_cast<size_t>(highlighted_top_level) &&
            (highlighted_nested < 0 || points[i].nested_index == highlighted_nested);
    }

    auto draw_marker = [&](size_t i) {
        // Off the visible widget -- skipped so it isn't an invisible click target.
        if (screen_points[i].x < widget_min.x || screen_points[i].x > widget_max.x ||
            screen_points[i].y < widget_min.y || screen_points[i].y > widget_max.y) {
            return;
        }

        const float radius = (highlighted[i] ? 13.0f : 10.0f) * scale;
        draw_list->AddCircleFilled(screen_points[i], radius,
                                   highlighted[i] ? IM_COL32(255, 130, 30, 255) : IM_COL32(35, 35, 35, 230));
        draw_list->AddCircle(screen_points[i], radius, IM_COL32(255, 130, 30, 255), 0,
                             (highlighted[i] ? 2.5f : 1.5f) * scale);

        char label[16];
        std::snprintf(label, sizeof(label), "%d", static_cast<int>(i) + 1);
        const ImVec2 label_size = ImGui::CalcTextSize(label);
        const ImU32 label_color = highlighted[i] ? IM_COL32(35, 35, 35, 255) : IM_COL32(255, 255, 255, 255);
        draw_list->AddText(ImVec2(screen_points[i].x - label_size.x * 0.5f, screen_points[i].y - label_size.y * 0.5f),
                           label_color, label);

        if (mouse_clicked && ImGui::IsMouseHoveringRect(
                ImVec2(screen_points[i].x - radius, screen_points[i].y - radius),
                ImVec2(screen_points[i].x + radius, screen_points[i].y + radius))) {
            click.clicked = true;
            click.top_level_index = points[i].top_level_index;
            click.nested_index = points[i].nested_index;
        }
    };

    // Two passes so a highlighted marker (and its hit-test) always wins over plain ones stacked at the same spot.
    for (size_t i = 0; i < screen_points.size(); ++i) {
        if (!highlighted[i]) { draw_marker(i); }
    }
    for (size_t i = 0; i < screen_points.size(); ++i) {
        if (highlighted[i]) { draw_marker(i); }
    }

    draw_list->PopClipRect();
    return click;
}

void DrawUavPositionMarker(Location &location, double uav_lat, double uav_lon,
                           double center_lat, double center_lon, int zoom,
                           ImVec2 widget_pos, float width, float height, float visible_h, float scale)
{
    ImVec2 screen_pos = location.latLonToScreenPos(uav_lat, uav_lon, center_lat, center_lon,
                                                    widget_pos, width, height, scale, zoom);
    if (screen_pos.x < widget_pos.x || screen_pos.x > widget_pos.x + width ||
        screen_pos.y < widget_pos.y || screen_pos.y > widget_pos.y + visible_h) {
        return;
    }

    ImDrawList *draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddCircleFilled(screen_pos, 6.0f * scale, IM_COL32(255, 50, 50, 255));
    draw_list->AddCircle(screen_pos, 6.0f * scale, IM_COL32(255, 255, 255, 255), 12, 1.5f * scale);
}
