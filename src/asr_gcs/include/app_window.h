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
#include "transformations.h"

enum class RTK_STATUS {
    INACTIVE = 0,
    SURVEY_IN = 1,
    STREAMING = 2
};

struct DroneInformation {
    BatteryState battery_values_M;
    BatteryState battery_values_C;
    double motor_speed[4] = {0.0, 0.0, 0.0, 0.0};
    float xyz_pos[3] = {0.0, 0.0, 0.0};
    float tgt_pos[3] = {0.0, 0.0, 0.0};
    float velocity[3] = {0.0, 0.0, 0.0};
    EulerAngles orientation;
    FlightMode flight_mode = FlightMode::STANDBY;
    GPSState gps_status;
    bool bs_connected = false;
    RTK_STATUS bs_status = RTK_STATUS::INACTIVE;

};



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
        ImFont* font14 = nullptr;
        ImFont* font18 = nullptr;
        ImFont* font18B = nullptr;
        ImFont* font24 = nullptr;
        ImFont* font28 = nullptr;
        ImFont* font40 = nullptr;
};

struct Color {
        static ImVec4 bgColor(bool theme = 0);
        static ImVec4 panelColor(bool theme = 0);
        static ImU32 panelBorder(bool theme = 0);
        static ImU32 dBlue_lGrey(bool theme = 0);
        static ImU32 white_black(bool theme = 0);
        static ImU32  dwhite_lblack(bool theme = 0);
};
