#include "camera.h"
#include "info_panels.h"  
#include "app_window.h"    

GLuint UploadCameraFrameToGL(const cv::Mat& img)
{
    if (img.empty()) {
        return 0;
    }

    cv::Mat rgba;
    switch (img.channels()) {
        case 1: cv::cvtColor(img, rgba, cv::COLOR_GRAY2RGBA); break;
        case 3: cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);  break;
        case 4: cv::cvtColor(img, rgba, cv::COLOR_BGRA2RGBA); break;
        default:
            std::cerr << "Unexpected channel count (" << img.channels() << ") in camera frame" << std::endl;
            return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);

    return tex;
}

void CameraFeedPanel(ImVec2 pos, float scale, bool theme, GLuint camera_tex)
{
    ImVec2 size = ImVec2(640 * scale, 480 * scale);
    BeginFixedPanel("CameraFeedPanel", pos, size, scale, theme, 0, ImVec2(0, 0));

    if (camera_tex != 0) {
        ImGui::Image((ImTextureID)(intptr_t)camera_tex, ImVec2(size.x, size.y));
    } else {
        ImVec2 center = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddText(center, Color::dwhite_lblack(theme), "No camera feed");
    }

    EndFixedPanel();
}