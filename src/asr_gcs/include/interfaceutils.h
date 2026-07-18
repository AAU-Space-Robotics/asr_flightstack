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
#include <list>  



namespace windowVar {
    extern int monitor_w;
    extern int monitor_h;
    extern int display_w;
    extern int display_h;
    extern ImVec4 BackgroundColor;
}

struct Color {
        static ImVec4 bgColor(bool theme = 0);
        static ImVec4 panelColor(bool theme = 0);
        static ImU32 panelBorder(bool theme = 0);
        static ImU32 dBlue_lGrey(bool theme = 0);
        static ImU32 white_black(bool theme = 0);


};

struct Widgets {
    bool costum_square_button(const char* id, ImVec2 pos, ImVec2 size, ImFont* font, float font_size, ImU32 color);
    bool costum_round_button(ImVec2 center, float radius, int segments, ImU32 color);
    bool DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, float scale, ImVec2 center, float radius, const char* id, float font_size);
    bool CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex, bool theme);
    GLuint LoadButtonImage(const char* path);
    void AltitudeTape(int direction, float altitude, float numStep, bool theme);
};

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


void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);



class Location {
    public:
        GLuint display_map(const char* path, float scale);
        ImVec2 latLonToTileOffset(double lat, double lon, int zoom);
        GLuint loadTileCached(int zoom, int x, int y);
        void MapWidget(double lat, double lon, float width, float height, float scale, int zoom = 12, GLuint placeholdetTile = 0, bool theme = 0);

    private:
        struct TileCoord { int x, y; };
        TileCoord latLonToTile(double lat, double lon, int zoom);
        std::unordered_map<std::string, GLuint> tileCache;
        std::list<std::string> tileLRU;
        std::unordered_map<std::string, std::list<std::string>::iterator> lruPos;
        static constexpr size_t MAX_CACHED_TILES = 300; // tune to taste
    
};

class TestFunc {
    public:
        void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);
};

