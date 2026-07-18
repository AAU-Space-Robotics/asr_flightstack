#include <GLFW/glfw3.h>
#include "interfaceutils.h"

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

bool Widgets::CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex, bool theme){
    // Compute bounding box
    float size_x = 26 * scale;
    float size_y = 26 * scale;
    ImVec2 bb_min = ImVec2(center.x - size_x, center.y - size_y);
    ImVec2 bb_max = ImVec2(center.x + size_x, center.y + size_y);
    float rounding = 12.0f;
    ImVec2 image_bb_min = ImVec2(center.x - 13, center.y - 13);
    ImVec2 image_bb_max = ImVec2(center.x + 13, center.y + 13);
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
void Widgets::AltitudeTape(int direction, float altitude, float numStep = 1.0f, bool theme = 0)
{
    switch (direction)
    {
    case 1:
    {
        /* code */
    ImGui::BeginChild("AltitudeTape_V", ImVec2(50, 120), true);
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    float centerY = pos.y + size.y / 2.0f;
   
    // How many numbers above/below center to draw
    const float pixelsPerTick = 14.0f;
    int range = 10;

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

    draw->AddRectFilled(boxMin, boxMax, IM_COL32(230, 126, 34, 255), 4.0f);  // orange, matches target

    draw->AddText(
        ImGui::GetFont(),
        14.0f,
        ImVec2(boxMin.x + boxPadX, centerY - centerTextSize.y / 2),
        IM_COL32(0, 0, 0, 255),  // black text on orange, matches target contrast
        centerBuf
    );

    ImGui::EndChild();
    break;
    }
    case 2:
    {
        ImGui::BeginChild("AltitudeTape_H", ImVec2(100, 80), true);
    
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
static GLuint UploadImageToGL(const cv::Mat& img, const char* path)
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

    std::string path = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/tiles/" //!!!!
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
    ImGui::BeginChild("MapWidget", ImVec2(width, height), false);
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

ImU32 Color::panelBorder(bool theme){
    if (theme) {
        return IM_COL32(43, 43, 82, 200);      // Dusk
    } else {
        return IM_COL32(143, 146, 166, 200);   // Steel
    }
}
