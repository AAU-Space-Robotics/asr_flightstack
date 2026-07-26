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
#include "app_window.h"

extern WindowInitializer winInit;

struct panelInfo {
    ImVec2 pos;
    ImVec2 size;
    int id;
};

float map_value(float value, float in_min, float in_max, float out_min, float out_max);


struct Graphs {
    void battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            float scale);
};


class InfoPanels {
    public:
        void ResetPanelTracking(); 
        void Battery_Info(float scale, bool theme, float battery_percentage[2]);
        void Position_Info(float scale, bool theme, float xyz_pos[3]);
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