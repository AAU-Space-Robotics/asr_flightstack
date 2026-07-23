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
#include <vector>
#include "statemanager.h"



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
        static ImU32  dwhite_lblack(bool theme = 0);
};

struct panelInfo {
    ImVec2 pos;
    ImVec2 size;
    int id;
};

float map_value(float value, float in_min, float in_max, float out_min, float out_max);
class Widgets {
    public:
        bool costum_square_button(const char* id, ImVec2 pos, ImVec2 size, ImFont* font, float font_size, ImU32 color);
        bool costum_round_button(ImVec2 center, float radius, int segments, ImU32 color);
        bool DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, float scale, ImVec2 center, float radius, const char* id, float font_size);
        bool CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex, bool theme, int but_size, int img_size);
        bool ArmButton(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, bool arming_state);
        GLuint LoadButtonImage(const char* path);
        void AltitudeTape(int direction, float altitude, float numStep, bool theme);
        void GyroScopeIndicator(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme);
        void Compas(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme);
    private:
        static std::vector<ImVec2> ArcPoints(float radius, float angleStart, float angleEnd, int segments);
};

struct Graphs {
    void battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            float scale);
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
        ImFont* font14 = nullptr;
        ImFont* font18 = nullptr;
        ImFont* font18B = nullptr;
        ImFont* font24 = nullptr;
        ImFont* font28 = nullptr;
        ImFont* font40 = nullptr;
};
extern WindowInitializer winInit;
ImU32 DarkenColor(ImU32 col, float factor);

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
        static constexpr size_t MAX_CACHED_TILES = 300; 
    
};

class TestFunc {
    public:
        void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);
};
class InfoPanels {
    public:
        void ResetPanelTracking(); 
        void Battery_Info(float scale, bool theme, float battery_percentage[2]);
        void Position_Info(float scale, bool theme = 0);
        void Probe_Info(float scale, bool theme = 0);
    private:
        ImVec2 Panel_tracker(ImVec2 size, float scale);
        int cur_panel_space = 0;
        std::list<panelInfo> tracker;
        ImVec2 Begin_panels(const char* id, int y_size, float scale, bool theme);
        void End_panels();
        bool CollapseButton(ImDrawList* draw_list, ImVec2 pos, float scale, bool& isOpen, bool theme);
};

void BeginFixedPanel(const char* id, ImVec2 pos, ImVec2 size, float scale, bool theme,
                      ImGuiWindowFlags extraFlags = 0, ImVec2 padding = ImVec2(8, 8));
void EndFixedPanel();

void BeginOverlayPanel(ImDrawList* draw_list, const char* id, ImVec2 pos, ImVec2 size,
                        float scale, bool theme,
                        ImGuiWindowFlags extraFlags = 0, ImVec2 padding = ImVec2(8, 8));
void EndOverlayPanel();

void DrawPanelBackground(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
                          ImU32 bg_color, ImU32 border_color,
                          float rounding, float border_thickness);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);