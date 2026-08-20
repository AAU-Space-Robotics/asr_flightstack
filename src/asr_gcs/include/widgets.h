#pragma once


#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "imgui_internal.h"
#include <opencv2/opencv.hpp>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <unordered_map>
#include <string>
#include <list>  
#include <vector>

#include "state_manager.h"
#include "app_window.h"
#include "manual.h"



extern WindowInitializer winInit;



class Widgets {
public:
    bool ModeToggle(ImDrawList* draw_list, ImVec2 center, const UiScale& scale, bool theme, bool& is_manual);
    bool costum_round_button(ImVec2 center, float radius, int segments, ImU32 color);
    bool DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, const UiScale& scale, ImVec2 center, float radius, const char* id, float font_size);
    bool CustomButton(ImDrawList* draw_list, ImVec2 center, const char* label, const UiScale& scale, GLuint tex, bool theme, int but_size, int img_size);
    static bool DrawSmallRedButton(ImDrawList* draw_list, ImVec2 center, const UiScale& scale, const char* label);
    bool ArmButton(ImDrawList* draw_list, ImVec2 center, const UiScale& scale, bool theme, bool arming_state);
    GLuint LoadButtonImage(const char* path);
    void AltitudeTape(int direction, float altitude, float numStep, bool theme, const UiScale& scale);
    void GyroScopeIndicator(ImDrawList* draw_list, ImVec2 center, const EulerAngles& orientation, bool theme, const UiScale& scale);
    void Compas(ImDrawList* draw_list, ImVec2 center, const EulerAngles& orientation, bool theme, const UiScale& scale);
    std::string GotoField(const UiScale& scale, bool theme);
    void GotoPanel(ImVec2 pos, const UiScale& scale, bool theme, std::string& goto_text);
    bool GotoButton(ImDrawList* draw_list, ImVec2 center, const UiScale& scale, bool theme, GLuint tex);
    void SurveyPanel(ImDrawList* draw_list, ImVec2 pos, const UiScale& scale, bool theme, RTK_STATUS rtk_survey, bool basestate_connected, JoystickState js, const LinkStatus& link_status);
private:
    static std::vector<ImVec2> ArcPoints(float radius, float angleStart, float angleEnd, int segments);
};

struct WeatherData {
    double temperature = 0.0;
    int wind_speed_level = 0;      //  1-8 Beaufort-like scale, not raw m/s
    std::string wind_direction;    // "N", "NW", etc. -- already text
    int cloud_cover = 0;           // 1-9 scale
    std::string precip_type;       // "none", "rain", "snow", etc.
    std::string condition;         // e.g. "clear", "cloudy"
    bool valid = false;
};


WeatherData FetchWeather(double lat, double lon);