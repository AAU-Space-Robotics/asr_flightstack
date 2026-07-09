#pragma once


#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "imgui_internal.h"
#include <opencv2/opencv.hpp>

#include <cmath>
#include <unordered_map>
#include <string>

static std::unordered_map<std::string, GLuint> tileCache;

namespace windowVar {
    extern int monitor_w;
    extern int monitor_h;
    extern int display_w;
    extern int display_h;
    extern ImVec4 BackgroundColor;
}

class WindowInitializer {
public:
    
    void GetPrimaryMonitorResolution(int& width, int& height);
    void Setup();
    void UpdateWindowSize(float scale);
    
    void Render();
    void DrawMultiColor();
    void loadFonts();     // load all fonts once
    ImFont* getFont(int size);

private:
    ImFont* font18 = nullptr;
    ImFont* font24 = nullptr;
    ImFont* font28 = nullptr;
    ImFont* font40 = nullptr;
};
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
ImU32 DarkenColor(ImU32 col, float factor);
class Widgets {
public:
    bool costum_square_button(const char* id, ImVec2 pos, ImVec2 size, ImFont* font, float font_size, ImU32 color);
    bool costum_round_button(ImVec2 center, float radius, int segments, ImU32 color);
    bool DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, float scale, ImVec2 center, float radius, const char* id, float font_size);
    bool CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex);
    GLuint LoadButtonImage(const char* path);
};

void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);

void AltitudeTape(int direction, float altitude, float tapeHeight, float numStep);

class Location {
    private:

        struct TileCoord { int x, y; };
        TileCoord latLonToTile(double lat, double lon, int zoom);
    public:
        GLuint display_map(const char* path, float scale);
        ImVec2 latLonToTileOffset(double lat, double lon, int zoom);
        GLuint loadTileCached(int zoom, int x, int y);
        void MapWidget(double lat, double lon, float width, float height, float scale, int zoom = 12);

        
};

class TestFunc {
    public:
        void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);
};
