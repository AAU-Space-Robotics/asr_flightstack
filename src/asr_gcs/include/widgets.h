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