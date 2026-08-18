#pragma once
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>

GLuint UploadCameraFrameToGL(const cv::Mat& img);
void CameraFeedPanel(ImVec2 pos, float scale, bool theme, GLuint camera_tex);