#include "widgets.h"
#include "app_window.h"

typedef void (APIENTRY *PFNGLGENERATEMIPMAPPROC)(unsigned int target);
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap_ptr = nullptr;


bool Widgets::ModeToggle(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, bool& is_manual)
{
    float seg_w = 90.0f * scale;    // half-width of EACH segment
    float seg_h = 18.0f * scale;
    float rounding = 6.0f * scale;

    ImVec2 outer_min = ImVec2(center.x - seg_w, center.y - seg_h);
    ImVec2 outer_max = ImVec2(center.x + seg_w, center.y + seg_h);

    // Background pill (the "unactivated" dark container)
    draw_list->AddRectFilled(outer_min, outer_max,
        ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)), rounding);
    draw_list->AddRect(outer_min, outer_max,
        Color::panelBorder(theme), rounding, 0, 1.5f * scale);

    // Each half's bounding box
    ImVec2 auto_min   = outer_min;
    ImVec2 auto_max   = ImVec2(center.x, outer_max.y);
    ImVec2 manual_min = ImVec2(center.x, outer_min.y);
    ImVec2 manual_max = outer_max;

    // Orange highlight slides to whichever side is active
    ImU32 highlight_color;
    if (is_manual) {
        highlight_color = IM_COL32(77, 130, 219, 255);
        draw_list->AddRectFilled(manual_min, manual_max, highlight_color, rounding, ImDrawFlags_RoundCornersRight);
    } else {
        highlight_color = IM_COL32(230, 126, 34, 255); 
        draw_list->AddRectFilled(auto_min, auto_max, highlight_color, rounding, ImDrawFlags_RoundCornersLeft);
    }

    // Independent hover/click zones for each half
    bool hovered_auto   = ImGui::IsMouseHoveringRect(auto_min, auto_max);
    bool hovered_manual = ImGui::IsMouseHoveringRect(manual_min, manual_max);
    bool clicked = false;

    if (hovered_auto && ImGui::IsMouseReleased(0))   { is_manual = false; clicked = true; }
    if (hovered_manual && ImGui::IsMouseReleased(0)) { is_manual = true;  clicked = true; }

    // Labels — active side gets black text (sits on orange), inactive gets dim white/black
    ImGui::PushFont(winInit.getFont(181));
    const char* auto_txt   = "AUTO";
    const char* manual_txt = "MANUAL";

    ImVec2 auto_text_size   = ImGui::CalcTextSize(auto_txt);
    ImVec2 manual_text_size = ImGui::CalcTextSize(manual_txt);

    ImU32 auto_color   = is_manual ? Color::dwhite_lblack(theme) : IM_COL32(0, 0, 0, 255);
    ImU32 manual_color = is_manual ? IM_COL32(0, 0, 0, 255)      : Color::dwhite_lblack(theme);

    ImVec2 auto_center   = ImVec2((auto_min.x + auto_max.x) / 2, (auto_min.y + auto_max.y) / 2);
    ImVec2 manual_center = ImVec2((manual_min.x + manual_max.x) / 2, (manual_min.y + manual_max.y) / 2);

    draw_list->AddText(ImVec2(auto_center.x - auto_text_size.x / 2, auto_center.y - auto_text_size.y / 2), auto_color, auto_txt);
    draw_list->AddText(ImVec2(manual_center.x - manual_text_size.x / 2, manual_center.y - manual_text_size.y / 2), manual_color, manual_txt);
    ImGui::PopFont();

    return clicked;
}


bool Widgets::costum_round_button(ImVec2 center, float radius, int segments, ImU32 color)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 bb_min = ImVec2(center.x - radius, center.y - radius);
    ImVec2 bb_max = ImVec2(center.x + radius, center.y + radius);
    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool released = hovered && ImGui::IsMouseReleased(0);
    draw_list->AddCircleFilled(center, radius, color, segments);
    return released;
}

bool Widgets::CustomButton(ImDrawList* draw_list, ImVec2 center,
                            const char* label,float scale, 
                            GLuint tex, bool theme, int but_size, int img_size){
    float size_x = (26 + but_size) * scale;
    float size_y = (26 + but_size) * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 12.0f;
    ImVec2 image_bb_min = ImVec2(center.x - (13 * scale + img_size), center.y - (13 * scale + img_size));
    ImVec2 image_bb_max = ImVec2(center.x + (13 * scale + img_size), center.y + (13 * scale + img_size));
    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);
    bool released = hovered && ImGui::IsMouseReleased(0);

    ImVec4 normal_color  = Color::panelColor(theme);
    ImVec4 hovered_color = ImVec4(
        normal_color.x + 0.15f,
        normal_color.y + 0.15f,
        normal_color.z + 0.15f,
        normal_color.w
    );
    ImVec4 active_color = ImVec4(
        normal_color.x * 0.7f,
        normal_color.y * 0.7f,
        normal_color.z * 0.7f,
        normal_color.w
    );

    ImVec4 base_color = active ? active_color :
                        hovered ? hovered_color : normal_color;

    float padding = 5.0f * scale;

    draw_list->AddRectFilled(bb_min, bb_max, ImGui::ColorConvertFloat4ToU32(base_color), rounding);

    draw_list->AddImageRounded(
        (ImTextureID)(intptr_t)tex,
        ImVec2(image_bb_min.x + padding, image_bb_min.y + padding),
        ImVec2(image_bb_max.x - padding, image_bb_max.y - padding),
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, 200),
        rounding
    );

    return released;
}
bool Widgets::DrawSmallRedButton(ImDrawList* draw_list, ImVec2 center, float scale, const char* label)
{
    float size_x = 40 * scale;
    float size_y = 15 * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 6.0f * scale;

    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);
    bool released = hovered && ImGui::IsMouseReleased(0);

    ImVec4 normal_color  = ImVec4(0.7f, 0.15f, 0.15f, 1.0f);
    ImVec4 hovered_color = ImVec4(
        normal_color.x + 0.15f,
        normal_color.y + 0.15f,
        normal_color.z + 0.15f,
        normal_color.w
    );
    ImVec4 active_color = ImVec4(
        normal_color.x * 0.7f,
        normal_color.y * 0.7f,
        normal_color.z * 0.7f,
        normal_color.w
    );

    ImVec4 base_color = active ? active_color : hovered ? hovered_color : normal_color;

    draw_list->AddRectFilled(bb_min, bb_max, ImGui::ColorConvertFloat4ToU32(base_color), rounding);

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(center.x - text_size.x / 2, center.y - text_size.y / 2);
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), label);

    return released;
}

bool Widgets::ArmButton(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, bool arming_state){
    float rounding = 8.0f;
    float size_x = 60 * scale;
    float size_y = 20 * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);
    bool released = hovered && ImGui::IsMouseReleased(0);
    ImVec4 normal_color = ImVec4(0.902f, 0.494f, 0.133f, 1.0f);
    ImVec4 hovered_color = ImVec4(
        normal_color.x + 0.15f,
        normal_color.y + 0.15f,
        normal_color.z + 0.15f,
        normal_color.w
    );
    ImVec4 active_color = ImVec4(
        normal_color.x * 0.7f,
        normal_color.y * 0.7f,
        normal_color.z * 0.7f,
        normal_color.w
    );

    int displace_txt = 18 * scale;
    const char* arming_txt;
    if(arming_state){
        arming_txt = "Disarm";
        displace_txt = 32 * scale;
    } else {
        arming_txt = "Arm";
    }
    ImVec4 base_color = active ? active_color :
                        hovered ? hovered_color : normal_color;

    draw_list->AddRectFilled(bb_min, bb_max, ImGui::ColorConvertFloat4ToU32(base_color), rounding);
    ImGui::PushFont(winInit.getFont(24));
    draw_list->AddText(ImVec2(center.x - displace_txt, center.y - 12 * scale), IM_COL32(0,0,0,255), arming_txt);
    ImGui::PopFont();
    return released;
}

bool Widgets::DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, float scale, ImVec2 center, float radius,const char* label, float font_size)
{
    ImVec2 bb_min = ImVec2(center.x - radius, center.y - radius);
    ImVec2 bb_max = ImVec2(center.x + radius, center.y + radius);

    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);

    ImVec4 white_color  = ImVec4(0.9f, 0.1f, 0.1f, 0.3f);
    ImVec4 estop_color  = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 hovered_color= ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
    ImVec4 active_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    ImVec4 base_color = active ? active_color :
                        hovered ? hovered_color : estop_color;

    const int num_segments = 64;

    for (int i = 0; i < num_segments; i++)
    {
        float t = (float)i / (num_segments - 1);

        ImVec4 c;
        c.x = white_color.x + (base_color.x - white_color.x) * t;
        c.y = white_color.y + (base_color.y - white_color.y) * t;
        c.z = white_color.z + (base_color.z - white_color.z) * t;
        c.w = white_color.w + (base_color.w - white_color.w) * t;

        float r = radius * (1.0f - t);

        draw_list->AddCircleFilled(
            center,
            r,
            ImGui::ColorConvertFloat4ToU32(c),
            64
        );
    }

    draw_list->AddCircle(
        center,
        radius,
        IM_COL32(0, 0, 0, 255),
        64,
        3.0f * scale
    );

    ImGui::PushFont(font);

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(
        center.x - text_size.x * 0.5f,
        center.y - text_size.y * 0.5f
    );

    draw_list->AddText(text_pos, IM_COL32(255,255,255,255), label);

    ImGui::PopFont();

    return active;
}

void Widgets::AltitudeTape(int direction, float altitude, float numStep, bool theme, float scale)
{
    switch (direction)
    {
    case 1:
    {
        ImGui::BeginChild("AltitudeTape_V", ImVec2(50 * scale, 120 *scale), true,
                            ImGuiWindowFlags_NoScrollbar|
                            ImGuiWindowFlags_NoScrollWithMouse);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();

        float centerY = pos.y + size.y / 2.0f;

        const float pixelsPerTick = 14.0f;
        int range = 8;

        float nearest = roundf(altitude / numStep) * numStep;
        float offset = (altitude - nearest) * (pixelsPerTick / numStep);

        for (int i = -range; i <= range; i++)
        {
            float value = nearest - i * numStep;
            float y = centerY + (i * pixelsPerTick) + offset;
            if (y < pos.y || y > pos.y + size.y) continue;

            char buf[16];
            snprintf(buf, sizeof(buf), "%.2f", value);
            ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(10.0f, FLT_MAX, 0.0f, buf);

            draw->AddLine(
                ImVec2(pos.x, y),
                ImVec2(pos.x + size.x * 0.2f, y),
                Color::white_black(theme),
                1.0f
            );

            draw->AddText(
                ImGui::GetFont(),
                10.0f * scale,
                ImVec2(pos.x + size.x * 0.34f + 2, y - textSize.y / 2),
                Color::white_black(theme),
                buf
            );
        }

        char centerBuf[16];
        snprintf(centerBuf, sizeof(centerBuf), "%.1f", altitude);
        ImVec2 centerTextSize = ImGui::GetFont()->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, centerBuf);

        float boxPadX = 8.0f * scale;
        float boxPadY = 4.0f * scale;
        ImVec2 boxMin = ImVec2(pos.x + size.x * 0.25f, centerY - centerTextSize.y / 2 - boxPadY);
        ImVec2 boxMax = ImVec2(pos.x + size.x, centerY + centerTextSize.y / 2 + boxPadY);

        draw->AddRectFilled(boxMin, boxMax, IM_COL32(230, 126, 34, 255), 4.0f);

        draw->AddText(
            ImGui::GetFont(),
            14.0f * scale,
            ImVec2(boxMin.x + boxPadX, centerY - centerTextSize.y / 2),
            IM_COL32(0, 0, 0, 255),
            centerBuf
        );

        ImGui::EndChild();
        break;
    }
    case 2:
    {
        ImGui::BeginChild("AltitudeTape_H", ImVec2(100, 80), true,
                            ImGuiWindowFlags_NoScrollbar|
                            ImGuiWindowFlags_NoScrollWithMouse);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        float centerX = pos.x + size.x / 2.0f;
        const float pixelsPerTick = 40.0f;
        int range = 10;

        float nearest = roundf(altitude / numStep) * numStep;
        float offset = (altitude - nearest) * (pixelsPerTick / numStep);
        for (int i = -range; i <= range; i++)
        {
            float value = nearest - i * numStep;
            float x = centerX + (i * pixelsPerTick) + offset;

            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", value);

            draw->AddText(
                ImVec2(x - ImGui::CalcTextSize(buf).x / 2, pos.y + 20),
                Color::white_black(theme),
                buf
            );

            draw->AddLine(
                ImVec2(x, pos.y + 5),
                ImVec2(x, pos.y+18),
                Color::white_black(theme),
                2.0f
            );
        }

        draw->AddLine(
            ImVec2(centerX, pos.y),
            ImVec2(centerX, pos.y + size.y),
            IM_COL32(255, 255, 0, 255),
            3.0f
        );
        ImGui::EndChild();
        break;
    }

    default:
        break;
    }
}

std::vector<ImVec2> Widgets::ArcPoints(float radius, float angleStart, float angleEnd, int segments) {
    std::vector<ImVec2> pts;
    pts.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / (float)segments;
        float a = angleStart + (angleEnd - angleStart) * t;
        pts.push_back(ImVec2(cosf(a) * radius, sinf(a) * radius));
    }
    return pts;
}

void Widgets::GyroScopeIndicator(ImDrawList* draw_list,ImVec2 center, const EulerAngles& orientation, bool theme, float scale){

    const float radius = 50 * scale;
    const int segments = 32;

    ImU32 sky_color    = IM_COL32(40, 80, 150, 255);
    ImU32 ground_color = IM_COL32(110, 70, 40, 255);
    ImU32 horizon_line  = IM_COL32(255, 210, 40, 255);
    ImU32 wing_color    = IM_COL32(255, 210, 40, 255);
    ImU32 pointer_color = IM_COL32(255, 130, 30, 255);
    ImU32 white = IM_COL32(255, 255, 255, 255);

    float theta = -orientation.roll * (IM_PI / 180.0f);
    float pixels_per_degree = radius / 25.0f;
    float k = orientation.pitch * pixels_per_degree;
    float yawTheta = -orientation.roll * (IM_PI / 180.0f);

    auto toScreen = [&](ImVec2 p){
        return ImVec2(
            center.x + p.x * cosf(theta) - p.y * sinf(theta),
            center.y + p.x * sinf(theta) + p.y * cosf(theta)
        );
    };

    auto toScreenYaw = [&](ImVec2 p) {
        return ImVec2(
            center.x + p.x * cosf(yawTheta) - p.y * sinf(yawTheta),
            center.y + p.x * sinf(yawTheta) + p.y * cosf(yawTheta)
        );
    };

    std::vector<ImVec2> skyPoly, groundPoly;
    ImVec2 chordL, chordR;
    bool hasChord = false;

    if ( k >= radius) {
        groundPoly = ArcPoints(radius, 0.0f, 2.0f * IM_PI, segments);
    } else if (k <= -radius)  {
        groundPoly = ArcPoints(radius, 0.0f, 2.0f * IM_PI, segments);
    } else {
        float x0 = sqrtf(radius * radius - k * k);
        chordL = ImVec2(-x0, k);
        chordR = ImVec2(x0, k);
        hasChord = true;

        float angleR = atan2f(k, x0);
        float angleL = IM_PI - angleR;

        groundPoly =  ArcPoints(radius, angleR, angleL, segments);
        skyPoly =  ArcPoints(radius, angleL, angleR + 2.0f * IM_PI, segments);
    }

    if (!skyPoly.empty()) {
        for (auto& p : skyPoly) p = toScreen(p);
        draw_list -> AddConvexPolyFilled(skyPoly.data(), (int)skyPoly.size(), sky_color);
    }
    if (!groundPoly.empty()) {
        for (auto& p : groundPoly) p = toScreen(p);
        draw_list->AddConvexPolyFilled(groundPoly.data(), (int)groundPoly.size(), ground_color);
    }

    if (hasChord) {
        draw_list->AddLine(toScreen(chordL), toScreen(chordR), horizon_line, 2.0f);
    }

    auto rotatePoint = [&](float dx, float dy) {
        float a = yawTheta;
        return ImVec2(
            center.x + dx * cosf(a) - dy * sinf(a),
            center.y + dx * sinf(a) + dy * cosf(a)
        );
    };

    draw_list->AddLine(rotatePoint(-(radius - 35 * scale), -30.0f * scale), rotatePoint(radius - 35 * scale, -30.0f * scale), white, 1.5f * scale);
    draw_list->AddLine(rotatePoint(-(radius - 45 * scale), -15.0f * scale), rotatePoint(radius - 45 * scale, -15.0f * scale), white, 1.5f * scale);
    draw_list->AddLine(rotatePoint(-(radius - 35 * scale),  30.0f * scale), rotatePoint(radius - 35 * scale,  30.0f * scale), white, 1.5f * scale);
    draw_list->AddLine(rotatePoint(-(radius - 45 * scale),  15.0f * scale), rotatePoint(radius - 45 * scale,  15.0f * scale), white, 1.5f * scale);

    const float tickSpacingDeg = 15.0f;
    const float shortTick = 5.0f * scale;
    const float longTick  = 10.0f * scale;

    for (int i = 1; i <= 4; ++i) {
        float tickLen = (i % 2 == 0) ? longTick : shortTick;

        for (int side = -1; side <= 1; side += 2) {
            float a = side * i * tickSpacingDeg * (IM_PI / 180.0f) + yawTheta;

            ImVec2 dir = ImVec2(sinf(a), -cosf(a));

            ImVec2 outer = ImVec2(center.x + dir.x * radius, center.y + dir.y * radius);
            ImVec2 inner = ImVec2(center.x + dir.x * (radius - tickLen), center.y + dir.y * (radius - tickLen));

            draw_list->AddLine(outer, inner, white, 1.5f);
        }
    }
    ImVec2 start_line = toScreenYaw(ImVec2(0.0f, -radius));
    ImVec2 end_line = toScreenYaw(ImVec2(0.0f, -radius + 10.0f * scale));
    draw_list->AddLine(start_line, end_line, white, 1.5f);

    ImVec2 tip_yaw = ImVec2(center.x, center.y - radius + 2.0f * scale);
    ImVec2 baseL_yaw = ImVec2(center.x - 6.0f * scale, center.y - radius + 10.0f * scale);
    ImVec2 baseR_yaw = ImVec2(center.x + 6.0f * scale, center.y - radius + 10.0f * scale);

    float wing = 30.0f * scale, gap = 6.0f * scale;
    draw_list->AddLine(ImVec2(center.x - wing, center.y), ImVec2(center.x - gap, center.y), wing_color, 3.5f);
    draw_list->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + wing, center.y), wing_color, 3.5f);
    draw_list->AddCircleFilled(center, 2.0f, wing_color);

    draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 64, 3.0f);
}

void Widgets::Compas(ImDrawList* draw_list,ImVec2 center, const EulerAngles& orientation, bool theme, float scale){

    const float radius = 50 * scale;
    float yawTheta = orientation.yaw * (IM_PI / 180.0f);
    ImU32 pointer_color = IM_COL32(255, 130, 30, 255);

    draw_list->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)), 64);
    auto toScreenYaw = [&](ImVec2 p) {
        return ImVec2(
            center.x + p.x * cosf(yawTheta) - p.y * sinf(yawTheta),
            center.y + p.x * sinf(yawTheta) + p.y * cosf(yawTheta)
        );
    };

    const float tickSpacingDeg = 30.0f;
    const float tickLen = 10.0f;

    int tickCount = 12;

    for (int i = 0; i < tickCount; ++i) {
        float a = i * tickSpacingDeg * (IM_PI / 180.0f);

        ImVec2 dir = ImVec2(sinf(a), -cosf(a));

        ImVec2 outer = ImVec2(center.x + dir.x * radius,           center.y + dir.y * radius);
        ImVec2 inner = ImVec2(center.x + dir.x * (radius - tickLen), center.y + dir.y * (radius - tickLen));

        draw_list->AddLine(outer, inner, Color::white_black(theme), 1.5f);
    }

    draw_list->AddLine(ImVec2(center.x - radius, center.y), ImVec2(center.x - radius + 13, center.y), Color::white_black(theme), 3.0f);
    draw_list->AddLine(ImVec2(center.x + radius, center.y), ImVec2(center.x + radius - 13, center.y), Color::white_black(theme), 3.0f);
    draw_list->AddLine(ImVec2(center.x, center.y + radius), ImVec2(center.x, center.y + radius - 13), Color::white_black(theme), 3.0f);
    draw_list->AddLine(ImVec2(center.x, center.y - radius), ImVec2(center.x, center.y - radius + 13), Color::white_black(theme), 3.0f);

    struct CardinalLabel { const char* text; float angleDeg; ImU32 color; };
    CardinalLabel labels[] = {
        { "N", 0.0f,   pointer_color },
        { "E", 90.0f,  Color::white_black(theme) },
        { "S", 180.0f, Color::white_black(theme) },
        { "W", 270.0f, Color::white_black(theme) },
    };

    float labelRadius = radius - 22.0f;

    for (auto& lbl : labels) {

        float a = (lbl.angleDeg * (IM_PI / 180.0f));

        ImVec2 dir = ImVec2(sinf(a), -cosf(a));
        ImVec2 pos = ImVec2(center.x + dir.x * labelRadius, center.y + dir.y * labelRadius);

        ImVec2 textSize = ImGui::CalcTextSize(lbl.text);
        ImVec2 drawPos = ImVec2(pos.x - textSize.x * 0.5f, pos.y - textSize.y * 0.5f);

        draw_list->AddText(drawPos, lbl.color, lbl.text);
    }

    ImVec2 tip_yaw   = toScreenYaw(ImVec2(0.0f, -radius + 2.0f));
    ImVec2 baseL_yaw = toScreenYaw(ImVec2(-6.0f, -radius + 10.0f));
    ImVec2 baseR_yaw = toScreenYaw(ImVec2(6.0f, -radius + 10.0f));
    draw_list->AddTriangleFilled(tip_yaw, baseL_yaw, baseR_yaw, pointer_color);

    char headingStr[8];
    snprintf(headingStr, sizeof(headingStr), "%.0f°", fmodf(orientation.yaw + 360.0f, 360.0f));

    ImVec2 textSize = ImGui::CalcTextSize(headingStr);
    ImVec2 textPos = ImVec2((center.x - textSize.x * 0.5f) + 2 * scale, (center.y - textSize.y * 0.5f) + 10 * scale);

    draw_list->AddText(textPos, Color::white_black(theme), headingStr);
    draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 64, 3.0f);
}

static GLuint UploadButtonImageToGL(const cv::Mat& img, const char* path, int* outW = nullptr, int* outH = nullptr)
{
    if (img.empty()) {
        std::cerr << "Failed to load image: " << path << std::endl;
        return 0;
    }

    cv::Mat rgba;
    switch (img.channels()) {
        case 1: cv::cvtColor(img, rgba, cv::COLOR_GRAY2RGBA); break;
        case 3: cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);  break;
        case 4: cv::cvtColor(img, rgba, cv::COLOR_BGRA2RGBA); break;
        default:
            std::cerr << "Unexpected channel count (" << img.channels() << ") in " << path << std::endl;
            return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);

    if (!glGenerateMipmap_ptr) {
        glGenerateMipmap_ptr = (PFNGLGENERATEMIPMAPPROC)glfwGetProcAddress("glGenerateMipmap");
    }
    if (glGenerateMipmap_ptr) {
        glGenerateMipmap_ptr(GL_TEXTURE_2D);
    } else {
        std::cerr << "glGenerateMipmap not available on this GL context" << std::endl;
    }

    if (outW) *outW = rgba.cols;
    if (outH) *outH = rgba.rows;

    return tex;
}

GLuint Widgets::LoadButtonImage(const char* path)
{
    cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
    return UploadButtonImageToGL(img, path);
}

void Widgets::GotoPanel(ImVec2 pos, float scale, bool theme, std::string& goto_text){ 

    ImVec2 size =ImVec2(420 * scale, 160 * scale);
    ImGui::SetCursorPos(ImVec2(pos.x * scale, pos.y * scale)); 
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * scale, 8 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::BeginChild("GotoPanel", size, true,
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();

    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((windowPos.x + 10 * scale), (windowPos.y + 10 * scale)), Color::white_black(theme), "Command");
    ImGui::PopFont();

    draw->AddText(ImVec2(windowPos.x + 10 * scale, windowPos.y + 50 * scale),
        Color::dwhite_lblack(theme), "Position");
    
    ImGui::SetCursorPos(ImVec2(10 * scale, 75 * scale));   
    goto_text = GotoField(scale, theme); 
    

    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

}

std::string Widgets::GotoField(float scale, bool theme)
{
    static char text_buffer[64] = "";
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Color::bgColor(theme));   // matches your other panels' background convention
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Color::bgColor(theme));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Color::bgColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f * scale);
    
    ImGui::SetNextItemWidth(250 * scale);
    ImGui::InputTextWithHint("##goto_input", "     x    y    z    yaw", text_buffer, sizeof(text_buffer));


    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    return std::string(text_buffer);   
}

bool Widgets::GotoButton(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, GLuint tex){
    float size_x = 35 * scale;
    float size_y = 15 * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 12.0f;
    ImVec2 image_bb_min = ImVec2(center.x - (13 * scale), center.y - (13 * scale));
    ImVec2 image_bb_max = ImVec2(center.x + (13 * scale ), center.y + (13 * scale));
    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);
    bool released = hovered && ImGui::IsMouseReleased(0);
    ImU32 pointer_color = IM_COL32(255, 130, 30, 255);

    ImVec4 normal_color  = Color::panelColor(theme);
    ImVec4 hovered_color = ImVec4(
        normal_color.x + 0.15f,
        normal_color.y + 0.15f,
        normal_color.z + 0.15f,
        normal_color.w
    );
    ImVec4 active_color = ImVec4(
        normal_color.x * 0.7f,
        normal_color.y * 0.7f,
        normal_color.z * 0.7f,
        normal_color.w
    );
    ImVec4 base_color = active ? active_color : hovered ? hovered_color : normal_color;
    draw_list->AddRectFilled(bb_min, bb_max, ImGui::ColorConvertFloat4ToU32(base_color), rounding);
    float icon_size = 13.0f * scale;
    draw_list->AddImageRounded(
        (ImTextureID)(intptr_t)tex,
        ImVec2(center.x - icon_size, center.y - icon_size),
        ImVec2(center.x + icon_size, center.y + icon_size),
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, 255),
        rounding
    );


    return released;
}


void Widgets::SurveyPanel(ImDrawList* draw_list, ImVec2 pos, float scale, bool theme, RTK_STATUS rtk_survey, bool bs_connected, JoystickState js, const LinkStatus& link_status){

    ImVec4 inactive_color     = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // grey — inactive/not connected
    ImVec4 survey_color       = ImVec4(0.9f, 0.6f, 0.0f, 1.0f);  // orange — survey in progress
    ImVec4 streaming_color    = ImVec4(0.1f, 0.7f, 0.2f, 1.0f);  // green — streaming/fixed
    ImVec4 notconnected_color = inactive_color;
    ImVec4 connected_color    = streaming_color;

    constexpr float kDotToLabelGap = 15.0f;  
    constexpr float kEntryGap      = 25.0f;  

    float cursor_x = 0.0f;
    auto StatusDot = [&](ImVec4 color, const char* label) {
        ImVec2 center((pos.x + cursor_x) * scale, (pos.y + 8) * scale);
        draw_list->AddCircleFilled(center, 6.0f * scale, ImGui::ColorConvertFloat4ToU32(color));
        draw_list->AddCircle(center, 6.0f * scale, IM_COL32(0, 0, 0, 255), 0, 1.5f * scale);

        const float text_x = cursor_x + kDotToLabelGap;
        draw_list->AddText(ImVec2((pos.x + text_x) * scale, pos.y * scale), Color::white_black(theme), label);

        const float label_width = ImGui::CalcTextSize(label).x / scale;
        cursor_x = text_x + label_width + kEntryGap;
    };

    ImVec4 rtk_survey_color;
    switch (rtk_survey) {
        case RTK_STATUS::INACTIVE:  rtk_survey_color = inactive_color;  break;
        case RTK_STATUS::SURVEY_IN: rtk_survey_color = survey_color;    break;
        case RTK_STATUS::STREAMING: rtk_survey_color = streaming_color; break;
    }
    StatusDot(link_status.mavlink_connected ? connected_color : notconnected_color, "Radio");
    StatusDot(link_status.wifi_connected    ? connected_color : notconnected_color, "WiFi");
    StatusDot(rtk_survey_color, "RTK-Survey");
    StatusDot(link_status.camera_streaming  ? connected_color : notconnected_color, "Camera");
    StatusDot(js.connected                  ? connected_color : notconnected_color, "Controller");
    
}

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total = size * nmemb;
    userp->append((char*)contents, total);
    return total;
}

WeatherData FetchWeather(double lat, double lon) {
    WeatherData result;

    CURL* curl = curl_easy_init();
    if (!curl) return result;

    std::string url = "https://www.7timer.info/bin/api.pl?lon=" + std::to_string(lon) +
                       "&lat=" + std::to_string(lat) +
                       "&product=civil&output=json";

    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);   // 7Timer can be slower than Open-Meteo
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Weather fetch failed: " << curl_easy_strerror(res) << std::endl;
        return result;
    }

    try {
        auto j = nlohmann::json::parse(response_body);
        auto first = j["dataseries"][0];   // nearest forecast point (next 3h)

        result.temperature = first["temp2m"].get<double>();
        result.cloud_cover = first["cloudcover"].get<int>();
        result.wind_speed_level = first["wind10m"]["speed"].get<int>();
        result.wind_direction = first["wind10m"]["direction"].get<std::string>();
        result.precip_type = first["prec_type"].get<std::string>();
        result.condition = first["weather"].get<std::string>();
        result.valid = true;
    } catch (const std::exception& e) {
        std::cerr << "Weather JSON parse failed: " << e.what() << std::endl;
    }

    return result;
}