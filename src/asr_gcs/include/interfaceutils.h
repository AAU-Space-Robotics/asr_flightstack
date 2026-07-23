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







class TestFunc {
    public:
        void scroll_wheel(ImDrawList* draw_list, float startx, float starty, float width, float height, float scale);
};