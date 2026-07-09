#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "interfaceutils.h"

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

    font18 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Regular.ttf", 18.0f);
    font24 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Bold.ttf", 24.0f);
    font28 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Regular.ttf", 28.0f);
    font40 = io.Fonts->AddFontFromFileTTF("src/asr_gcs/fonts/Roboto-Bold.ttf", 40.0f);

    io.Fonts->Build();

    io.FontDefault = font18;   // set default font

}
ImFont* WindowInitializer::getFont(int size)        
{
    switch (size) {
        case 18:
            return font18;
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

bool Widgets::CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex){
    // Compute bounding box
    float size_x = 40 * scale;
    float size_y = 40 * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 12.0f;

    bool hovered = ImGui::IsMouseHoveringRect(bb_min, bb_max);
    bool active = hovered && ImGui::IsMouseDown(0);

    ImVec4 normal_color   = ImVec4(0.20f, 0.20f, 0.45f, 1.0f);  // AAU blue 
    ImVec4 hovered_color = ImVec4(0.35f, 0.35f, 0.65f, 1.0f);  // hover lighter
    ImVec4 active_color  = ImVec4(0.20f, 0.20f, 0.45f, 1.0f); // same as base when clicked

    ImVec4 base_color = active ? active_color :
                        hovered ? hovered_color : normal_color;

    float padding = 5.0f * scale;
    draw_list->AddRectFilled(bb_min, bb_max, IM_COL32(255, 255, 255, 255), rounding);

    draw_list->AddImageRounded(
        (ImTextureID)(intptr_t)tex,
        ImVec2(bb_min.x + padding, bb_min.y + padding),  // inset min
        ImVec2(bb_max.x - padding, bb_max.y - padding),  // inset max
        ImVec2(0, 0), ImVec2(1, 1),
        IM_COL32(255, 255, 255, 255),
        rounding
    );
    draw_list->AddRect(
        ImVec2(center.x - size_x, center.y - size_y),
        ImVec2(center.x + size_x, center.y + size_y),
        ImGui::ColorConvertFloat4ToU32(base_color), 
        rounding, 
        0, 
        5.0f);

    return active;
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


void TestFunc::scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale) {

   
    draw_list->AddCircleFilled(ImVec2(startx, starty), 100.0f, IM_COL32(255, 255, 255, 255), 20); // Draw white circle at (400,200) with radius 30 



}
void AltitudeTape(int direction, float altitude, float tapeHeight = 300.0f, float numStep = 1.0f)
{
    switch (direction)
    {
    case 1:
    {
        /* code */
    ImGui::BeginChild("AltitudeTape_V", ImVec2(80, tapeHeight), true);
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    float centerY = pos.y + size.y / 2.0f;
   
    // How many numbers above/below center to draw
    const float pixelsPerTick = 40.0f;
    int range = 10;

    // Determine the altitude number nearest to center
    float nearest = roundf(altitude / numStep) * numStep;
    float offset = (altitude - nearest) * (pixelsPerTick / numStep); 
    

    for (int i = -range; i <= range; i++)
    {
        float value = nearest - i * numStep;
        float y = centerY + (i * pixelsPerTick) + offset;

        // Draw text centered
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", value);

        draw->AddText(
            ImVec2(pos.x + 20, y - ImGui::CalcTextSize(buf).y / 2),
            IM_COL32(255, 255, 255, 255),
            buf
        );

        // small horizontal tick mark
        draw->AddLine(
            ImVec2(pos.x + 5, y),
            ImVec2(pos.x + 18, y),
            IM_COL32(255, 255, 255, 255),
            2.0f
        );
    }

    // Draw the center reference line
    draw->AddLine(
        ImVec2(pos.x, centerY),
        ImVec2(pos.x + size.x, centerY),
        IM_COL32(255, 255, 0, 255),
        3.0f
    );
        ImGui::EndChild();
        break;
    }
    case 2:
    {
        ImGui::BeginChild("AltitudeTape_H", ImVec2(tapeHeight, 80), true);
    
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
                IM_COL32(255, 255, 255, 255),
                buf
            );
        
            // small horizontal tick mark
            draw->AddLine(
                ImVec2(x, pos.y + 5),
                ImVec2(x, pos.y+18),
                IM_COL32(255, 255, 255, 255),
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
GLuint Widgets::LoadButtonImage(const char* path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) {
        std::cout << "Failed to load button image: " << path << std::endl;
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return tex;
}

GLuint Location::display_map(const char* path, float scale){
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if(!data) return 0;

    GLuint image;
    glGenTextures(1, &image);
    glBindTexture(GL_TEXTURE_2D, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return image;
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
    if (it != tileCache.end()) return it->second;

    std::string path = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/tiles/" //! REMEMBER TO MAKE UNIVERSAL PATH
                       + key + ".png";
    GLuint tex = display_map(path.c_str(), 1.0f);  // your existing load function
    tileCache[key] = tex;
    return tex;
}

void Location::MapWidget(double lat, double lon, float width, float height, float scale, int zoom)
{
    ImGui::BeginChild("MapWidget", ImVec2(width, height), false);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();

    float tileSize = 256.0f * scale;

    TileCoord center = latLonToTile(lat, lon, zoom);
    ImVec2 offset = latLonToTileOffset(lat, lon, zoom);

    // How many tiles to draw in each direction
    int tilesX = (int)ceil(width  / tileSize / 2) + 1;
    int tilesY = (int)ceil(height / tileSize / 2) + 1;

    for (int dy = -tilesY; dy <= tilesY; dy++)
    for (int dx = -tilesX; dx <= tilesX; dx++)
    {
        int tx = center.x + dx;
        int ty = center.y + dy;

        // Screen position: center of widget, offset by tile grid, minus sub-tile offset
        float sx = pos.x + width  / 2.0f + dx * tileSize - offset.x * scale;
        float sy = pos.y + height / 2.0f + dy * tileSize - offset.y * scale;

        GLuint tex = loadTileCached(zoom, tx, ty);
        if (tex)
            draw->AddImage((ImTextureID)(intptr_t)tex,
                ImVec2(sx, sy),
                ImVec2(sx + tileSize, sy + tileSize));
    }

    // Drone dot at exact center
    float cx = pos.x + width  / 2.0f;
    float cy = pos.y + height / 2.0f;
    draw->AddCircleFilled(ImVec2(cx, cy), 6.0f * scale, IM_COL32(255, 50, 50, 255));
    draw->AddCircle(ImVec2(cx, cy), 6.0f * scale, IM_COL32(255, 255, 255, 255), 12, 1.5f);

    ImGui::EndChild();
}
