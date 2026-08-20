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

#include "state_manager.h"
#include "app_window.h"
#include "widgets.h"

extern WindowInitializer winInit;

struct panelInfo {
    ImVec2 pos;
    ImVec2 size;
    int id;
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

struct LogEntry {
    std::string timestamp;
    LogLevel level;
    std::string message;
};


struct LogFilters {
    bool show_info = true;
    bool show_warn = true;
    bool show_error = true;
    bool show_debug = false;
};
static LogFilters log_filters; 
struct Graphs {
    void battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            const UiScale& scale);
};
static float map_value(float value, float in_min, float in_max, float out_min, float out_max);

class InfoPanels {
public:
    void ResetPanelTracking();
    void Battery_Info(const UiScale& scale, bool theme, const BatteryState& battery_info_C, const BatteryState& battery_info_M, double motor_speeds[4]);
    void Position_Info(const UiScale& scale, bool theme, float xyz_pos[3], float target_meter[3], float vel_meter[3]);
    void Probe_Info(const UiScale& scale, bool theme, const ProbeData& info);
    void GNSS_Info(const UiScale& scale, bool theme,
        double accuracy,
        double accuracy_target,
        double duration,
        const GPSState& gps_info);
private:
    ImVec2 Panel_tracker(ImVec2 size, const UiScale& scale);
    int cur_panel_space = 0;
    std::list<panelInfo> tracker;
    ImVec2 Begin_panels(const char* id, int y_size, const UiScale& scale, bool theme);
    void End_panels();
    bool CollapseButton(ImDrawList* draw_list, ImVec2 pos, const UiScale& scale, bool& isOpen, bool theme);
};

void BeginFixedPanel(const char* id, ImVec2 pos, ImVec2 size, const UiScale& scale, bool theme,
                      ImGuiWindowFlags extraFlags = 0, ImVec2 padding = ImVec2(8, 8),
                      bool allow_scroll = false);
void EndFixedPanel();

void BeginOverlayPanel(ImDrawList* draw_list, const char* id, ImVec2 pos, ImVec2 size,
                        const UiScale& scale, bool theme,
                        ImGuiWindowFlags extraFlags = 0, ImVec2 padding = ImVec2(8, 8));
void EndOverlayPanel();

void DrawPanelBackground(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
                          ImU32 bg_color, ImU32 border_color,
                          float rounding, float border_thickness);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

struct Logs {
    bool outputlog(ImVec2 pos, const UiScale& scale, bool theme, const std::vector<LogEntry>& log_lines, const LogFilters& filters);
};