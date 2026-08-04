#include "app_window.h"
#include <ament_index_cpp/get_package_share_directory.hpp>


namespace windowVar {
    int monitor_w = 0;
    int monitor_h = 0;
    int display_w = 0;
    int display_h = 0;
    ImVec4 BackgroundColor = ImVec4(0.0, 0.0, 0.0, 0.0);
}


void WindowInitializer::DrawMultiColor() {
    ImVec2 window_pos(0, 0);
    ImVec2 window_size((float)windowVar::display_w, (float)windowVar::display_h);

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    ImU32 color_top_left = IM_COL32(0, 0, 100, 255);
    ImU32 color_top_right = IM_COL32(0, 0, 100, 255);
    ImU32 color_bottom_left = IM_COL32(0, 0, 20, 255);
    ImU32 color_bottom_right = IM_COL32(0, 0, 20, 255);

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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
}

void WindowInitializer::UpdateWindowSize(float scale) {
    windowVar::display_w = static_cast<int>(windowVar::monitor_w * scale);
    windowVar::display_h = static_cast<int>(windowVar::monitor_h * scale);
}

void WindowInitializer::Render() {
    // Rendering code here
}

void WindowInitializer::loadFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    const std::string fonts_dir = ament_index_cpp::get_package_share_directory("asr_gcs") + "/fonts/";
    const std::string regular = fonts_dir + "Roboto-Regular.ttf";
    const std::string bold = fonts_dir + "Roboto-Bold.ttf";

    font14 = io.Fonts->AddFontFromFileTTF(regular.c_str(), 14.0f);
    font18 = io.Fonts->AddFontFromFileTTF(regular.c_str(), 18.0f);
    font18B = io.Fonts->AddFontFromFileTTF(bold.c_str(), 18.0f);
    font24 = io.Fonts->AddFontFromFileTTF(bold.c_str(), 24.0f);
    font28 = io.Fonts->AddFontFromFileTTF(regular.c_str(), 28.0f);
    font40 = io.Fonts->AddFontFromFileTTF(bold.c_str(), 40.0f);

    io.Fonts->Build();
    io.FontDefault = font18;
}

ImFont* WindowInitializer::getFont(int size)
{
    switch (size) {
        case 14:  return font14;
        case 18:  return font18;
        case 181: return font18B;
        case 24:  return font24;
        case 28:  return font28;
        case 40:  return font40;
        default:  return font18;
    }
}

ImVec4 Color::bgColor(bool theme)
{
    if (theme) {
        return ImVec4(13 / 255.0f, 13 / 255.0f, 32 / 255.0f, 1.0f);
    } else {
        return ImVec4(208 / 255.0f, 209 / 255.0f, 216 / 255.0f, 1.0f);
    }
}

ImVec4 Color::panelColor(bool theme)
{
    if (theme) {
        return ImVec4(0.0824f, 0.0824f, 0.1843f, 1.0f);  // Midnight
    } else {
        return ImVec4(0.9333f, 0.9373f, 0.9529f, 1.0f);  // Pale grey
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

ImU32 Color::dBlue_lGrey(bool theme){
    if (theme) {
        return IM_COL32(43, 43, 82, 255);
    } else {
        return IM_COL32(200, 200, 210, 255);
    }
}