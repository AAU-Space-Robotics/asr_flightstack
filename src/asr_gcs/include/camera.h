#pragma once
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "app_window.h"

GLuint UploadCameraFrameToGL(const cv::Mat& img);
void CameraFeedPanel(ImVec2 pos, const UiScale& scale, bool theme, GLuint camera_tex);