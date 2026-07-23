#include <GLFW/glfw3.h>
#include "interfaceutils.h"
#include <ament_index_cpp/get_package_share_directory.hpp>

typedef void (APIENTRY *PFNGLGENERATEMIPMAPPROC)(unsigned int target);
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap_ptr = nullptr;

namespace windowVar {
    int monitor_w = 0;
    int monitor_h = 0;
    int display_w = 0;
    int display_h = 0;
    ImVec4 BackgroundColor = ImVec4(0.0, 0.0, 0.0, 0.0); // Transparent background in RGBA
}

void WindowInitializer::DrawMultiColor() {
    // Example function to demonstrate additional functionality
    ImVec2 window_pos(0, 0);
    ImVec2 window_size((float)windowVar::display_w, (float)windowVar::display_h);

    // Get draw list for the background
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    // Define colors (RGBA)
    ImU32 color_top_left = IM_COL32(0, 0, 100, 255);      
    ImU32 color_top_right = IM_COL32(0, 0, 100, 255);   
    ImU32 color_bottom_left = IM_COL32(0, 0, 20, 255);     
    ImU32 color_bottom_right = IM_COL32(0, 0, 20, 255);

    // Draw a multi-color rectangle (gradient effect)
    draw_list->AddRectFilledMultiColor(
        window_pos,
        ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
        color_top_left,
        color_top_right,
        color_bottom_right,
        color_bottom_left
    );
}

void WindowInitializer::GetPrimaryMonitorResolution(int& width, int& height) {
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    width = mode->width;
    height = mode->height;
    
}
void WindowInitializer::Setup() {
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup style
    ImGui::StyleColorsDark();
}

void WindowInitializer::UpdateWindowSize(float scale) {
    // Update the display size
    windowVar::display_w = static_cast<int>(windowVar::monitor_w * scale);
    windowVar::display_h = static_cast<int>(windowVar::monitor_h * scale);
}



void WindowInitializer::Render() {
    // Rendering code here
}

void WindowInitializer::loadFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    font14 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Regular.ttf", 14.0f);
    font18 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Regular.ttf", 18.0f);
    font18B = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Bold.ttf", 18.0f);
    font24 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Bold.ttf", 24.0f);
    font28 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Regular.ttf", 28.0f);
    font40 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Bold.ttf", 40.0f);

    io.Fonts->Build();

    io.FontDefault = font18;   // set default font

}
ImFont* WindowInitializer::getFont(int size)        
{
    switch (size) {
        case 14:
            return font14;
        case 18:
            return font18;
        case 181:
            return font18B;
        case 24:
            return font24;
        case 28:
            return font28;
        case 40:
            return font40;
        default:
            return font18; // default to font18 if size not found
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
} //This may not be in a class to work ??

float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

bool Widgets::costum_square_button(const char* id, ImVec2 pos, ImVec2 size, ImFont* font, float font_size, ImU32 color)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float rounding = 12.0f; // corner rounding radius
    // Button bounding box
    ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    // Detect hover / click manually
    bool hovered = rect.Contains(io.MousePos);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

 // Define corner colors (RGBA)
    ImU32 col_tr = DarkenColor(color, 0.4f); // slightly darker
    ImU32 col_br = DarkenColor(color, 0.4f);
    ImU32 col_tl = color; // keep original
    ImU32 col_bl = color;


    if (hovered)
    {
        // Example hover effect: lighten colors
        col_tl = DarkenColor(color, 1.2f);
        col_bl = DarkenColor(color, 1.2f);
        col_tr = DarkenColor(color, 1.2f);
        col_br = DarkenColor(color, 1.2f); 
    }

     //Draw colorful gradient rectangle
    draw_list->AddRectFilledMultiColor(
        rect.Min, rect.Max,
        col_tl,
        col_tr,
        col_bl,
        col_br
    );
    float border = 2.5f;

    ImVec2 border_rect_min = ImVec2(rect.Min.x - border, rect.Min.y - border);
    ImVec2 border_rect_max = ImVec2(rect.Max.x + border, rect.Max.y + border);
    draw_list->AddRect(border_rect_min, border_rect_max, IM_COL32(0, 0, 0, 255), rounding, 0, 3.2f);
    
    // Draw outline
    ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, id)
                             : ImGui::CalcTextSize(id);
    ImVec2 text_pos = ImVec2(
        pos.x + (size.x - text_size.x) * 0.5f,
        pos.y + (size.y - text_size.y) * 0.5f
    );

    // Draw text
    draw_list->AddText(font, font_size, text_pos, IM_COL32(255,255,255,255), id);
  

    return clicked;
}


ImU32 DarkenColor(ImU32 col, float factor)
{
    // Extract RGBA components
    unsigned char r = (col >> IM_COL32_R_SHIFT) & 0xFF;
    unsigned char g = (col >> IM_COL32_G_SHIFT) & 0xFF;
    unsigned char b = (col >> IM_COL32_B_SHIFT) & 0xFF;
    unsigned char a = (col >> IM_COL32_A_SHIFT) & 0xFF;

    // Darken by factor (0..1)
    r = (unsigned char)(r * factor);
    g = (unsigned char)(g * factor);
    b = (unsigned char)(b * factor);

    return IM_COL32(r, g, b, a);
}

bool Widgets::CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex, bool theme, int but_size, int img_size){
    // Compute bounding box
   
    float size_x = (26 + but_size) * scale;
    float size_y = (26 + but_size) * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 12.0f;
    ImVec2 image_bb_min = ImVec2(center.x - (13 + img_size), center.y - (13 + img_size));
    ImVec2 image_bb_max = ImVec2(center.x + (13 + img_size), center.y + (13 + img_size));
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
        ImVec2(image_bb_min.x + padding, image_bb_min.y + padding),  // inset min
        ImVec2(image_bb_max.x - padding, image_bb_max.y - padding),  // inset max
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, 200),
        rounding
    );
    

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
        displace_txt = 32;

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
    // Compute bounding box
    ImVec2 bb_min = ImVec2(center.x - radius, center.y - radius);
    ImVec2 bb_max = ImVec2(center.x + radius, center.y + radius);

    // Hover/active detection
    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);


    ImVec4 white_color  = ImVec4(0.9f, 0.1f, 0.1f, 0.3f);
    ImVec4 estop_color  = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 hovered_color= ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
    ImVec4 active_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    // State choose
    ImVec4 base_color = active ? active_color :
                        hovered ? hovered_color : estop_color;

    // Draw layered radial gradient 
    const int num_segments = 64;

    for (int i = 0; i < num_segments; i++)
    {
        float t = (float)i / (num_segments - 1);

        // Color interpolation
        ImVec4 c;
        c.x = white_color.x + (base_color.x - white_color.x) * t;
        c.y = white_color.y + (base_color.y - white_color.y) * t;
        c.z = white_color.z + (base_color.z - white_color.z) * t;
        c.w = white_color.w + (base_color.w - white_color.w) * t;

        float r = radius * (1.0f - t); // decreasing radius

        draw_list->AddCircleFilled(
            center,
            r,
            ImGui::ColorConvertFloat4ToU32(c),
            64
        );
    }

    // Outer border circle
    draw_list->AddCircle(
        center,
        radius,
        IM_COL32(0, 0, 0, 255), // white 0.5 alpha
        64,
        3.0f * scale
    );

    // Draw label in center
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



void Widgets::AltitudeTape(int direction, float altitude, float numStep = 1.0f, bool theme = 0)
{
    switch (direction)
    {
    case 1:
    {
  
    ImGui::BeginChild("AltitudeTape_V", ImVec2(50, 120), true,
                        ImGuiWindowFlags_NoScrollbar|
                        ImGuiWindowFlags_NoScrollWithMouse);
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    float centerY = pos.y + size.y / 2.0f;
   
    // How many numbers above/below center to draw
    const float pixelsPerTick = 14.0f;
    int range = 8;

    // Determine the altitude number nearest to center
    float nearest = roundf(altitude / numStep) * numStep;
    float offset = (altitude - nearest) * (pixelsPerTick / numStep); 
    

    for (int i = -range; i <= range; i++)
    {
        float value = nearest - i * numStep;
        float y = centerY + (i * pixelsPerTick) + offset;
        if (y < pos.y || y > pos.y + size.y) continue; 
        // Draw text centered
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", value);
        ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(10.0f, FLT_MAX, 0.0f, buf);

        draw->AddLine(
            ImVec2(pos.x, y),
            ImVec2(pos.x + size.x * 0.2f, y),   // ← extends further right
            Color::white_black(theme),
            1.0f
        );

        draw->AddText(
            ImGui::GetFont(),
            10.0f,
            ImVec2(pos.x + size.x * 0.34f + 2, y - textSize.y / 2),  // right after the tick
            Color::white_black(theme),
            buf
        );
    }

    char centerBuf[16];
    snprintf(centerBuf, sizeof(centerBuf), "%.1f", altitude);
    ImVec2 centerTextSize = ImGui::GetFont()->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, centerBuf);

    float boxPadX = 8.0f;
    float boxPadY = 4.0f;
    ImVec2 boxMin = ImVec2(pos.x + size.x * 0.25f, centerY - centerTextSize.y / 2 - boxPadY);
    ImVec2 boxMax = ImVec2(pos.x + size.x, centerY + centerTextSize.y / 2 + boxPadY);

    draw->AddRectFilled(boxMin, boxMax, IM_COL32(230, 126, 34, 255), 4.0f);  

    draw->AddText(
        ImGui::GetFont(),
        14.0f,
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

        // Determine the altitude number nearest to center
        float nearest = roundf(altitude / numStep) * numStep;
        float offset = (altitude - nearest) * (pixelsPerTick / numStep); 
        for (int i = -range; i <= range; i++)
        {
            float value = nearest - i * numStep;
            float x = centerX + (i * pixelsPerTick) + offset;
        
            // Draw text centered
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", value);
        
            draw->AddText(
                ImVec2(x - ImGui::CalcTextSize(buf).x / 2, pos.y + 20),
                Color::white_black(theme),
                buf
            );
        
            // small horizontal tick mark
            draw->AddLine(
                ImVec2(x, pos.y + 5),
                ImVec2(x, pos.y+18),
                Color::white_black(theme),
                2.0f
            );
        }
    
        // Draw the center reference line
       
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

void Widgets::GyroScopeIndicator(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme){

    const float radius = 50;
    const int segments = 32;

    ImU32 sky_color    = IM_COL32(40, 80, 150, 255);
    ImU32 ground_color = IM_COL32(110, 70, 40, 255);
    ImU32 horizon_line  = IM_COL32(255, 210, 40, 255);
    ImU32 wing_color    = IM_COL32(255, 210, 40, 255);
    ImU32 pointer_color = IM_COL32(255, 130, 30, 255);
    ImU32 white = IM_COL32(255, 255, 255, 255);

    float theta = -orientation.roll * (IM_PI / 180.0f);
    float pixels_per_degree = radius / 25.0f;
    float k = -orientation.pitch * pixels_per_degree;
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
        float a = yawTheta;  // roll angle, name pending your rename
        return ImVec2(
            center.x + dx * cosf(a) - dy * sinf(a),
            center.y + dx * sinf(a) + dy * cosf(a)
        );
    };

    draw_list->AddLine(rotatePoint(-(radius - 35), -30.0f), rotatePoint(radius - 35, -30.0f), white, 1.5f);
    draw_list->AddLine(rotatePoint(-(radius - 45), -15.0f), rotatePoint(radius - 45, -15.0f), white, 1.5f);
    draw_list->AddLine(rotatePoint(-(radius - 35),  30.0f), rotatePoint(radius - 35,  30.0f), white, 1.5f);
    draw_list->AddLine(rotatePoint(-(radius - 45),  15.0f), rotatePoint(radius - 45,  15.0f), white, 1.5f);

    const float tickSpacingDeg = 15.0f;  
    const float shortTick = 5.0f;
    const float longTick  = 10.0f;

    for (int i = 1; i <= 4; ++i) {
        float tickLen = (i % 2 == 0) ? longTick : shortTick;  

        for (int side = -1; side <= 1; side += 2) {   
            float a = side * i * tickSpacingDeg * (IM_PI / 180.0f) + yawTheta;  // + roll offset

            ImVec2 dir = ImVec2(sinf(a), -cosf(a));   

            ImVec2 outer = ImVec2(center.x + dir.x * radius, center.y + dir.y * radius);
            ImVec2 inner = ImVec2(center.x + dir.x * (radius - tickLen), center.y + dir.y * (radius - tickLen));

            draw_list->AddLine(outer, inner, white, 1.5f);
        }
    }
    ImVec2 start_line = toScreenYaw(ImVec2(0.0f, -radius));
    ImVec2 end_line = toScreenYaw(ImVec2(0.0f, -radius + 10.0f));
    draw_list->AddLine(start_line, end_line, white, 1.5f);

    ImVec2 tip_yaw = ImVec2(center.x, center.y - radius + 2.0f);
    ImVec2 baseL_yaw = ImVec2(center.x - 6.0f, center.y - radius + 10.0f);
    ImVec2 baseR_yaw = ImVec2(center.x + 6.0f, center.y - radius + 10.0f);
    draw_list->AddTriangleFilled(tip_yaw, baseL_yaw, baseR_yaw, pointer_color);

     // Fixed aircraft reference symbol
    float wing = 30.0f, gap = 6.0f;
    draw_list->AddLine(ImVec2(center.x - wing, center.y), ImVec2(center.x - gap, center.y), wing_color, 3.5f);
    draw_list->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + wing, center.y), wing_color, 3.5f);
    draw_list->AddCircleFilled(center, 2.0f, wing_color);

    draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 64, 3.0f);
}

void Widgets::Compas(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme){

    const float radius = 50;
    float yawTheta = orientation.yaw * (IM_PI / 180.0f);
    ImU32 pointer_color = IM_COL32(255, 130, 30, 255);

    draw_list->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)), 64);
    auto toScreenYaw = [&](ImVec2 p) {
        return ImVec2(
            center.x + p.x * cosf(yawTheta) - p.y * sinf(yawTheta),
            center.y + p.x * sinf(yawTheta) + p.y * cosf(yawTheta)
        );
    };

    const float tickSpacingDeg = 30.0f;   // 360 / 12 = 30° apart -> 12 ticks total
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
    ImVec2 textPos = ImVec2((center.x - textSize.x * 0.5f) + 2 , (center.y - textSize.y * 0.5f) + 10);

    draw_list->AddText(textPos, Color::white_black(theme), headingStr);
    draw_list->AddCircle(center, radius, IM_COL32(0, 0, 0, 255), 64, 3.0f);

}
static GLuint UploadImageToGL(const cv::Mat& img, const char* path)
{
    if (img.empty()) {
        //std::cerr << "Failed to load image: " << path << std::endl;
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

    // Force onto a fixed, fully-allocated 256x256 canvas regardless of what
    // the decoder actually produced — this makes an overread impossible,
    // even if rgba's real dimensions don't match what it claims.
    constexpr int TILE_SIZE = 256;
    cv::Mat canvas(TILE_SIZE, TILE_SIZE, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    int copyW = std::min(TILE_SIZE, rgba.cols);
    int copyH = std::min(TILE_SIZE, rgba.rows);
    if (copyW > 0 && copyH > 0) {
        rgba(cv::Rect(0, 0, copyW, copyH)).copyTo(canvas(cv::Rect(0, 0, copyW, copyH)));
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TILE_SIZE, TILE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, canvas.data);

    return tex;
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

GLuint Location::display_map(const char* path, float scale)
{
    cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
    return UploadImageToGL(img, path);
}
Location::TileCoord Location::latLonToTile(double lat, double lon, int zoom)
{
    int n = 1 << zoom;
    int x = (int)((lon + 180.0) / 360.0 * n);
    double latRad = lat * M_PI / 180.0;
    int y = (int)((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n);
    return {x, y};
}


ImVec2 Location::latLonToTileOffset(double lat, double lon, int zoom)
{
    int n = 1 << zoom;
    double xFrac = (lon + 180.0) / 360.0 * n;
    double latRad = lat * M_PI / 180.0;
    double yFrac = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;
    return { (float)((xFrac - floor(xFrac)) * 256),
             (float)((yFrac - floor(yFrac)) * 256) };
}

GLuint Location::loadTileCached(int zoom, int x, int y)
{
    std::string key = std::to_string(zoom) + "/" + std::to_string(x) + "/" + std::to_string(y);

    auto it = tileCache.find(key);
    if (it != tileCache.end()) {
        tileLRU.splice(tileLRU.begin(), tileLRU, lruPos[key]);
        return it->second;
    }
    std::string package_path = ament_index_cpp::get_package_share_directory("asr_gcs");
    std::string path = package_path + "/tiles/" 
                       + key + ".png";
  
    GLuint tex = display_map(path.c_str(), 1.0f);
    if (!tex) return 0; // missing tile — don't cache a failure as if it were real

    tileCache[key] = tex;
    tileLRU.push_front(key);
    lruPos[key] = tileLRU.begin();

    if (tileCache.size() > MAX_CACHED_TILES) {
        std::string oldestKey = tileLRU.back();
        GLuint oldTex = tileCache[oldestKey];
        glDeleteTextures(1, &oldTex);
        tileCache.erase(oldestKey);
        lruPos.erase(oldestKey);
        tileLRU.pop_back();
    }

    
    return tex;
}
void Location::MapWidget(double lat, double lon, float width, float height, float scale, int zoom, GLuint placeholderTile, bool theme)
{
    ImGui::BeginChild("MapWidget", ImVec2(width, height), false,
                        ImGuiWindowFlags_NoScrollbar|
                        ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();

    float tileSize = 256.0f * scale;

    TileCoord center = latLonToTile(lat, lon, zoom);
    ImVec2 offset = latLonToTileOffset(lat, lon, zoom);

    int tilesX = (int)ceil(width  / tileSize / 2) + 1;
    int tilesY = (int)ceil(height / tileSize / 2) + 1;

    for (int dy = -tilesY; dy <= tilesY; dy++)
    for (int dx = -tilesX; dx <= tilesX; dx++)
    {
        int tx = center.x + dx;
        int ty = center.y + dy;
        float sx = pos.x + width  / 2.0f + dx * tileSize - offset.x * scale;
        float sy = pos.y + height / 2.0f + dy * tileSize - offset.y * scale;
        GLuint tex = loadTileCached(zoom, tx, ty);
        if (tex) {
            draw->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(sx, sy),
                ImVec2(sx + tileSize, sy + tileSize));
        }
    }


    float roundRadius = 12.0f * scale;  // must match ChildRounding on the panel
    ImU32 maskColor = ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme));
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + width, pos.y + height);

    auto maskCorner = [&](ImVec2 outerCorner, float startAngle) {
        draw->PathClear();
        draw->PathLineTo(outerCorner);
        draw->PathArcTo(
            ImVec2(
                outerCorner.x + roundRadius * cosf(startAngle + IM_PI),
                outerCorner.y + roundRadius * sinf(startAngle + IM_PI)
            ),
            roundRadius, startAngle, startAngle + IM_PI * 0.5f, 8
        );
        draw->PathFillConvex(maskColor);
    };

    // Top-left
    draw->PathClear();
    draw->PathLineTo(p_min);
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_min.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_min.y + roundRadius), roundRadius, -IM_PI * 0.5f, -IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Top-right
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_max.x, p_min.y));
    draw->PathLineTo(ImVec2(p_max.x, p_min.y + roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_min.y + roundRadius), roundRadius, 0.0f, -IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-left
    draw->PathClear();
    draw->PathLineTo(ImVec2(p_min.x, p_max.y));
    draw->PathLineTo(ImVec2(p_min.x + roundRadius, p_max.y));
    draw->PathArcTo(ImVec2(p_min.x + roundRadius, p_max.y - roundRadius), roundRadius, IM_PI * 0.5f, IM_PI, 8);
    draw->PathFillConvex(maskColor);

    // Bottom-right
    draw->PathClear();
    draw->PathLineTo(p_max);
    draw->PathLineTo(ImVec2(p_max.x, p_max.y - roundRadius));
    draw->PathArcTo(ImVec2(p_max.x - roundRadius, p_max.y - roundRadius), roundRadius, 0.0f, IM_PI * 0.5f, 8);
    draw->PathFillConvex(maskColor);

    // Drone dot at exact center
    float cx = pos.x + width  / 2.0f;
    float cy = pos.y + height / 2.0f;
    draw->AddCircleFilled(ImVec2(cx, cy), 6.0f * scale, IM_COL32(255, 50, 50, 255));
    draw->AddCircle(ImVec2(cx, cy), 6.0f * scale, IM_COL32(255, 255, 255, 255), 12, 1.5f);

    ImGui::EndChild();
}


ImVec4 Color::bgColor(bool theme)
{
    if (theme) {
        return ImVec4(13 / 255.0f, 13 / 255.0f, 32 / 255.0f, 1.0f);   // darker navy
    } else {
        return ImVec4(208 / 255.0f, 209 / 255.0f, 216 / 255.0f, 1.0f); // light gray
    }
}
ImVec4 Color::panelColor(bool theme)
{
    if (theme) {
        return ImVec4(0.0824f, 0.0824f, 0.1843f, 1.0f);  // Midnight
    } else {
        return ImVec4(0.9333f, 0.9373f, 0.9529f, 1.0f);  //  Pale grey
    }
}

ImU32 Color::white_black(bool theme){
    if (theme) {
        return IM_COL32(255, 255, 255, 255);
    } else {
        return IM_COL32(0, 0, 0, 255);
    }
}

ImU32 Color::dwhite_lblack(bool theme){
    if (theme) {
        return IM_COL32(180, 180, 180, 255);
    } else {
        return IM_COL32(50, 50, 50, 255);
    }
}

ImU32 Color::panelBorder(bool theme){
    if (theme) {
        return IM_COL32(43, 43, 82, 200);      // Dusk
    } else {
        return IM_COL32(143, 146, 166, 200);   // Steel
    }
}

void DrawPanelBackground(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
                          ImU32 bg_color, ImU32 border_color,
                          float rounding, float border_thickness) {
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + size.x, pos.y + size.y);

    draw_list->AddRectFilled(p_min, p_max, bg_color, rounding);
    draw_list->AddRect(p_min, p_max, border_color, rounding, 0, border_thickness);
}


ImVec2 InfoPanels::Panel_tracker(ImVec2 size, float scale){

    const int max_panel_space = 1500;
    const float gap = 10.0f;
    if (cur_panel_space + size.y > max_panel_space){

        std::cout << "Damn boy" << std::endl;

    }

    ImVec2 panel_pos = ImVec2(1580* scale, (70 * scale + cur_panel_space)); 
    cur_panel_space += (size.y + gap);
    tracker.push_back({{1580, (float)cur_panel_space}, {size.x, size.y}, 0});
    return panel_pos;
}

void InfoPanels::ResetPanelTracking() {
    cur_panel_space = 0;
    tracker.clear();
}





ImVec2 InfoPanels::Begin_panels(const char* id, int y_size, float scale, bool theme){

    const ImVec2 size = ImVec2(310, y_size);
    ImVec2 pos = InfoPanels::Panel_tracker(size, scale);
    ImGui::SetCursorPos(ImVec2(pos.x, pos.y)); 
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * scale, 8 * scale)); 
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::BeginChild(id, size, true,
                        ImGuiWindowFlags_NoScrollbar|
                        ImGuiWindowFlags_NoScrollWithMouse);
    return pos;
}

void InfoPanels::End_panels(){
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginFixedPanel(const char* id, ImVec2 pos, ImVec2 size, float scale, bool theme,
                      ImGuiWindowFlags extraFlags, ImVec2 padding) {
    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale, padding.y * scale));
    ImGui::BeginChild(id, size, true,
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse |
                       extraFlags);
}

void EndFixedPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginOverlayPanel(ImDrawList* draw_list, const char* id, ImVec2 pos, ImVec2 size,
                        float scale, bool theme,
                        ImGuiWindowFlags extraFlags, ImVec2 padding) {
    DrawPanelBackground(draw_list, pos, size,
                         ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)),
                         Color::panelBorder(theme),
                         12.0f * scale, 2.0f * scale);

    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale, padding.y * scale));
    ImGui::BeginChild(id, size, false,
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse |
                       extraFlags);
}

void EndOverlayPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void Graphs::battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            float scale) {
    ImU32 color = IM_COL32(204, 204, 204, 255); // 0.8, 0.8, 0.8, 1.0

    draw_list->AddRect(ImVec2(x1 * scale, y1 * scale), ImVec2(x2 * scale, y2 * scale),
                        color, 1.0f * scale, /* flags=15 in Python — TODO confirm corners */ ImDrawFlags_RoundCornersAll,
                        3.0f * scale);

    draw_list->AddRect(ImVec2(x1b * scale, y1b * scale), ImVec2(x2b * scale, y2b * scale),
                        color, 1.0f * scale, /* flags=3 in Python — TODO confirm corners */ ImDrawFlags_RoundCornersAll,
                        3.0f * scale);
}

bool InfoPanels::CollapseButton(ImDrawList* draw_list, ImVec2 pos, float scale, bool& isOpen, bool theme){

    float size = 16.0f * scale;
    ImVec2 center = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f);

    // Invisible button for click detection
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##collapse", ImVec2(size, size));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    if (clicked) {
        isOpen = !isOpen;
    }

    ImU32 col = hovered ? Color::dwhite_lblack(theme) : Color::white_black(theme);
    float r = size * 0.5f;   

    if (isOpen) {
        // Chevron pointing down (expanded)
        draw_list->AddTriangleFilled(
            ImVec2(center.x - r, center.y - r * 0.5f),
            ImVec2(center.x + r, center.y - r * 0.5f),
            ImVec2(center.x, center.y + r * 0.5f),
            col);
    } else {
        // Chevron pointing up (collapsed)
        draw_list->AddTriangleFilled(
            ImVec2(center.x - r, center.y + r * 0.5f),
            ImVec2(center.x + r, center.y + r * 0.5f),
            ImVec2(center.x, center.y - r * 0.5f),
            col);
    }

    return clicked;
}

void InfoPanels::Battery_Info(float scale, bool theme, float battery_percentage[]){
    int size;
    static bool Motor_panel_open = true;
    if(Motor_panel_open) {
        size = 350 * scale;
    } else {
        size = 190 * scale;
    }

    ImVec2 pos = InfoPanels::Begin_panels("BatteryInfo", size, scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Power & Motors");
    ImGui::PopFont();

    const char* Battery_row_labels[2] = {"MOTORS", "COMPUTER"};
    ImU32 Battery_row_colors[2] = { IM_COL32(255, 140, 0, 255), IM_COL32(70, 150, 255, 255) };

    float Battery_values_motors[4]  = {battery_percentage[0], 0.0f, 0.0f, 0.0f};
    float Battery_values_compute[3] = {battery_percentage[1], 0.0f, 0.0f};
    float* Battery_values[2] = { Battery_values_motors, Battery_values_compute };

    const char* Battery_text[3] = {"Voltage","Discharge", "Avg. Current"};
    const char* Battery_text_value[3] = {"V", "mAh", "A"};

    float row_height = 70.0f * scale;
    float row_start_y = 45.0f * scale;

    for (int i = 0; i < 2; i++){
        float row_y = pos.y + row_start_y + (row_height * i);

        ImU32 battery_color;
        if (battery_percentage[i] > 0.5f) {
            battery_color = IM_COL32(0, 255, 0, 255);
        } else if (battery_percentage[i] > 0.25f) {
            battery_color = IM_COL32(255, 255, 0, 255);
        } else {
            battery_color = IM_COL32(255, 0, 0, 255);
        }

        // --- Battery icon ---
        float icon_x = pos.x + 15 * scale;
        float icon_top = row_y;
        float icon_bottom = row_y + 55 * scale;

        draw->AddRectFilled(
            ImVec2(icon_x + 9 * scale, icon_top - 6 * scale),
            ImVec2(icon_x + 21 * scale, icon_top),
            Battery_row_colors[i], 3.0f * scale, ImDrawFlags_RoundCornersTop);

        // Fill bar — slides between icon_bottom (empty) and icon_top (full)
        float battery_progressbar = map_value(battery_percentage[i], 0.0f, 1.0f,
                                               icon_bottom - 4.0f * scale,
                                               icon_top + 6.0f * scale);
        draw->AddRectFilled(
            ImVec2(icon_x + 3 * scale, battery_progressbar),
            ImVec2(icon_x + 27 * scale, icon_bottom - 2.0f * scale),
            battery_color, 3.0f * scale, ImDrawFlags_RoundCornersBottom);

        draw->AddRect(
            ImVec2(icon_x, icon_top),
            ImVec2(icon_x + 30 * scale, icon_bottom),
            Battery_row_colors[i], 6.0f * scale, ImDrawFlags_RoundCornersAll, 2.5f * scale);

        // --- Row label ---
        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 55 * scale, row_y + 2 * scale),
                      Battery_row_colors[i], Battery_row_labels[i]);
        ImGui::PopFont();

        // --- Percentage ---
        char pct_text[16];
        snprintf(pct_text, sizeof(pct_text), "%.0f%%", battery_percentage[i] * 100.0f);
        draw->AddText(ImVec2(pos.x + 260 * scale, row_y + 4 * scale),
                      Color::dwhite_lblack(theme), pct_text);

        // --- V / A / mAh values, tighter gap now ---
        float value_x = pos.x + 65 * scale;
        float value_col_spacing = 90.0f * scale;
        for (int j = 0; j < 3; j++){
            char value_text[16];
            snprintf(value_text, sizeof(value_text), "%.2f", Battery_values[i][j]);

            draw->AddText(ImVec2(value_x + (value_col_spacing * j), row_y + 25 * scale),
                          Color::dwhite_lblack(theme), value_text);
            draw->AddText(ImVec2(value_x + (value_col_spacing * j) + 30 * scale, row_y + 25 * scale), // was +40 -> +30
                          Color::dwhite_lblack(theme), Battery_text_value[j]);
        }

        if (i < 1) {
            draw->AddLine(
                ImVec2(pos.x + 14 * scale, row_y + row_height - 12 * scale),
                ImVec2(pos.x + 296 * scale, row_y + row_height - 12 * scale),
                Color::panelBorder(theme), 1.0f);
        }
    }  

    float motors_section_y = pos.y + row_start_y + (row_height * 2);

    draw->AddLine(
        ImVec2(pos.x + 14 * scale, motors_section_y),
        ImVec2(pos.x + 296 * scale, motors_section_y),
        Color::panelBorder(theme), 3.0f);

    InfoPanels::CollapseButton(draw, ImVec2(pos.x + 290 * scale, pos.y + 10 * scale), scale, Motor_panel_open, theme);

    if (Motor_panel_open) {
        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 15 * scale, motors_section_y + 15 * scale),
                      IM_COL32(255, 140, 0, 255), "MOTOR USAGE");
        ImGui::PopFont();

        float Motor_values[4] = {battery_percentage[0], 0.0f, 0.0f, 0.0f};
        const char* Motor_labels[4] = {"M1", "M2", "M3", "M4"};
        float Motor_row_spacing = 30.0f * scale;
        float bar_x_start = 45.0f * scale;
        float bar_width   = 180.0f * scale;
        float bar_height  = 8.0f * scale;
        ImU32 motor_color = IM_COL32(255, 140, 0, 255);

        for (int i = 0; i < 4; i++){
            float row_y = motors_section_y + 45.0f * scale + (Motor_row_spacing * i);

            draw->AddText(ImVec2(pos.x + 15 * scale, row_y),
                          Color::dwhite_lblack(theme), Motor_labels[i]);

            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale),
                ImVec2(pos.x + bar_x_start + bar_width, row_y + 3 * scale + bar_height),
                Color::panelBorder(theme), 4.0f * scale, ImDrawFlags_RoundCornersAll);

            float fill_width = map_value(Motor_values[i], 0.0f, 1.0f, 0.0f, bar_width);
            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale),
                ImVec2(pos.x + bar_x_start + fill_width, row_y + 3 * scale + bar_height),
                motor_color, 4.0f * scale, ImDrawFlags_RoundCornersAll);

            char motor_text[16];
            snprintf(motor_text, sizeof(motor_text), "%.0f%%", Motor_values[i] * 100.0f);
            draw->AddText(ImVec2(pos.x + bar_x_start + bar_width + 50 * scale, row_y),
                          Color::dwhite_lblack(theme), motor_text);
        }
    }

    InfoPanels::End_panels();
}
void InfoPanels::Position_Info(float scale, bool theme){

    ImVec2 pos = InfoPanels::Begin_panels("PositionInfo",210 * scale,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "State-NED");
    ImGui::PopFont();

    float pos_meter[3] = {2.1f, 20.3f, -10.5f};
    float target_meter[3] = {2.1f, 0.3f, -1.5f};
    float vel_meter[3] = {0.0f, 0.3f, -0.23f};
    ImU32 xyz_color[3] = {
        IM_COL32(232, 45, 39, 255),  
        IM_COL32(122, 193, 66, 255),   
        IM_COL32(44, 169, 225, 255),   
    };
    const char* xyz_text[3] = {"X", "Y", "Z"};
    const char* explain_text[3] = {"POS m", "TGT m", "Vel m/s"};
    int x_space = 75 * scale;
    int y_space = 30 * scale;
   for (int i = 0; i < 3; i++){
        draw->AddText(ImVec2(pos.x + 100 + (x_space * i) * scale, pos.y + 30 * scale),   
                                  xyz_color[i], xyz_text[i]);
        ImGui::PushFont(winInit.getFont(181));
        char pos_txt[16];
        snprintf(pos_txt, sizeof(pos_txt), "%.2f", pos_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 60 * scale),   
                                 Color::white_black(theme) , pos_txt);
        ImGui::PopFont();
        char tgt_txt[16];
        snprintf(tgt_txt, sizeof(tgt_txt), "%.2f", target_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 90 * scale),   
        Color::dwhite_lblack(theme), tgt_txt);
        char vel_txt[16];
        snprintf(vel_txt, sizeof(vel_txt), "%.2f", vel_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 120 * scale),   
        Color::dwhite_lblack(theme), vel_txt);

        draw->AddText(ImVec2(pos.x + 10 * scale, pos.y + 60 * scale  + (y_space *i) ),   
        Color::dwhite_lblack(theme), explain_text[i]);

    }
    draw->AddLine(
        ImVec2(pos.x + 14 * scale, pos.y + 155 * scale),
        ImVec2(pos.x + 296 * scale, pos.y + 155 * scale),
        Color::panelBorder(theme), 3.0f);

    float speed_numb = sqrt(pow(vel_meter[0], 2.0f) + pow(vel_meter[1],2.0f));
    char speed_char[8];
    snprintf(speed_char, sizeof(speed_char), "%.2f", speed_numb);
    draw->AddText(ImVec2((pos.x + 250 * scale), (pos.y + 170 * scale)), Color::dwhite_lblack(theme), speed_char);
    draw->AddText(ImVec2((pos.x + 10 * scale), (pos.y + 170 * scale)), Color::dwhite_lblack(theme), "Speed m/s");

    InfoPanels::End_panels();

}

void InfoPanels::Probe_Info(float scale, bool theme){

    ImVec2 pos = InfoPanels::Begin_panels("ProbeInfo",170,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Probe Info");

    


    
   
    InfoPanels::End_panels();
}