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


extern WindowInitializer winInit;
class Widgets {
    public:
        bool ModeToggle(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, bool& is_manual);
        bool costum_round_button(ImVec2 center, float radius, int segments, ImU32 color);
        bool DrawCircleGradientButton(ImDrawList* draw_list, ImFont* font, float scale, ImVec2 center, float radius, const char* id, float font_size);
        bool CustomButton(ImDrawList* draw_list, ImVec2 center,const char* label,float scale, GLuint tex, bool theme, int but_size, int img_size);
        bool ArmButton(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, bool arming_state);
        GLuint LoadButtonImage(const char* path);
        void AltitudeTape(int direction, float altitude, float numStep, bool theme, float scale);
        void GyroScopeIndicator(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme, float scale);
        void Compas(ImDrawList* draw_list,ImVec2 center, EulerAngles orientation, bool theme, float scale);
        std::string GotoField(float scale, bool theme);
        void GotoPanel(ImVec2 pos, float scale, bool theme, std::string& goto_text);
        bool GotoButton(ImDrawList* draw_list, ImVec2 center, float scale, bool theme, GLuint tex);
        void SurveryPanel(ImDrawList* draw_list, ImVec2 pos, float scale, float theme);
    private:
        static std::vector<ImVec2> ArcPoints(float radius, float angleStart, float angleEnd, int segments);
        
};