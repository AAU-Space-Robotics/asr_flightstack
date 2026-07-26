#include "info_panels.h"
#include "app_window.h"


float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Graphs::battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            float scale) {
    ImU32 color = IM_COL32(204, 204, 204, 255);

    draw_list->AddRect(ImVec2(x1 * scale, y1 * scale), ImVec2(x2 * scale, y2 * scale),
                        color, 1.0f * scale, ImDrawFlags_RoundCornersAll,
                        3.0f * scale);

    draw_list->AddRect(ImVec2(x1b * scale, y1b * scale), ImVec2(x2b * scale, y2b * scale),
                        color, 1.0f * scale, ImDrawFlags_RoundCornersAll,
                        3.0f * scale);
}

void DrawPanelBackground(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
                          ImU32 bg_color, ImU32 border_color,
                          float rounding, float border_thickness) {
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + size.x, pos.y + size.y);

    draw_list->AddRectFilled(p_min, p_max, bg_color, rounding);
    draw_list->AddRect(p_min, p_max, border_color, rounding, 0, border_thickness);
}

ImVec2 InfoPanels::Panel_tracker(ImVec2 size, float scale){

    const int max_panel_space = 1500;
    const float gap = 10.0f;
    if (cur_panel_space + size.y > max_panel_space){
        std::cout << "Damn boy" << std::endl;
    }

    ImVec2 panel_pos = ImVec2(1580* scale, (70 * scale + cur_panel_space));
    cur_panel_space += (size.y + gap);
    tracker.push_back({{1580, (float)cur_panel_space}, {size.x, size.y}, 0});
    return panel_pos;
}

void InfoPanels::ResetPanelTracking() {
    cur_panel_space = 0;
    tracker.clear();
}

ImVec2 InfoPanels::Begin_panels(const char* id, int y_size, float scale, bool theme){

    const ImVec2 size = ImVec2(310, y_size);
    ImVec2 pos = InfoPanels::Panel_tracker(size, scale);
    ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * scale, 8 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::BeginChild(id, size, true,
                        ImGuiWindowFlags_NoScrollbar|
                        ImGuiWindowFlags_NoScrollWithMouse);
    return pos;
}

void InfoPanels::End_panels(){
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginFixedPanel(const char* id, ImVec2 pos, ImVec2 size, float scale, bool theme,
                      ImGuiWindowFlags extraFlags, ImVec2 padding) {
    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale, padding.y * scale));
    ImGui::BeginChild(id, size, true,
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse |
                       extraFlags);
}

void EndFixedPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginOverlayPanel(ImDrawList* draw_list, const char* id, ImVec2 pos, ImVec2 size,
                        float scale, bool theme,
                        ImGuiWindowFlags extraFlags, ImVec2 padding) {
    DrawPanelBackground(draw_list, pos, size,
                         ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)),
                         Color::panelBorder(theme),
                         12.0f * scale, 2.0f * scale);

    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale, padding.y * scale));
    ImGui::BeginChild(id, size, false,
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse |
                       extraFlags);
}

void EndOverlayPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

bool InfoPanels::CollapseButton(ImDrawList* draw_list, ImVec2 pos, float scale, bool& isOpen, bool theme){

    float size = 16.0f * scale;
    ImVec2 center = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f);

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("##collapse", ImVec2(size, size));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    if (clicked) {
        isOpen = !isOpen;
    }

    ImU32 col = hovered ? Color::dwhite_lblack(theme) : Color::white_black(theme);
    float r = size * 0.5f;

    if (isOpen) {
        draw_list->AddTriangleFilled(
            ImVec2(center.x - r, center.y - r * 0.5f),
            ImVec2(center.x + r, center.y - r * 0.5f),
            ImVec2(center.x, center.y + r * 0.5f),
            col);
    } else {
        draw_list->AddTriangleFilled(
            ImVec2(center.x - r, center.y + r * 0.5f),
            ImVec2(center.x + r, center.y + r * 0.5f),
            ImVec2(center.x, center.y - r * 0.5f),
            col);
    }

    return clicked;
}

void InfoPanels::Battery_Info(float scale, bool theme, float battery_percentage[]){
    int size;
    static bool Motor_panel_open = true;
    if(Motor_panel_open) {
        size = 350 * scale;
    } else {
        size = 190 * scale;
    }

    ImVec2 pos = InfoPanels::Begin_panels("BatteryInfo", size, scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Power & Motors");
    ImGui::PopFont();

    const char* Battery_row_labels[2] = {"MOTORS", "COMPUTER"};
    ImU32 Battery_row_colors[2] = { IM_COL32(255, 140, 0, 255), IM_COL32(70, 150, 255, 255) };

    float Battery_values_motors[4]  = {battery_percentage[0], 0.0f, 0.0f, 0.0f};
    float Battery_values_compute[3] = {battery_percentage[1], 0.0f, 0.0f};
    float* Battery_values[2] = { Battery_values_motors, Battery_values_compute };

    const char* Battery_text[3] = {"Voltage","Discharge", "Avg. Current"};
    const char* Battery_text_value[3] = {"V", "mAh", "A"};

    float row_height = 70.0f * scale;
    float row_start_y = 45.0f * scale;

    for (int i = 0; i < 2; i++){
        float row_y = pos.y + row_start_y + (row_height * i);

        ImU32 battery_color;
        if (battery_percentage[i] > 0.5f) {
            battery_color = IM_COL32(0, 255, 0, 255);
        } else if (battery_percentage[i] > 0.25f) {
            battery_color = IM_COL32(255, 255, 0, 255);
        } else {
            battery_color = IM_COL32(255, 0, 0, 255);
        }

        float icon_x = pos.x + 15 * scale;
        float icon_top = row_y;
        float icon_bottom = row_y + 55 * scale;

        draw->AddRectFilled(
            ImVec2(icon_x + 9 * scale, icon_top - 6 * scale),
            ImVec2(icon_x + 21 * scale, icon_top),
            Battery_row_colors[i], 3.0f * scale, ImDrawFlags_RoundCornersTop);

        float battery_progressbar = map_value(battery_percentage[i], 0.0f, 1.0f,
                                               icon_bottom - 4.0f * scale,
                                               icon_top + 6.0f * scale);
        draw->AddRectFilled(
            ImVec2(icon_x + 3 * scale, battery_progressbar),
            ImVec2(icon_x + 27 * scale, icon_bottom - 2.0f * scale),
            battery_color, 3.0f * scale, ImDrawFlags_RoundCornersBottom);

        draw->AddRect(
            ImVec2(icon_x, icon_top),
            ImVec2(icon_x + 30 * scale, icon_bottom),
            Battery_row_colors[i], 6.0f * scale, ImDrawFlags_RoundCornersAll, 2.5f * scale);

        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 55 * scale, row_y + 2 * scale),
                      Battery_row_colors[i], Battery_row_labels[i]);
        ImGui::PopFont();

        char pct_text[16];
        snprintf(pct_text, sizeof(pct_text), "%.0f%%", battery_percentage[i] * 100.0f);
        draw->AddText(ImVec2(pos.x + 260 * scale, row_y + 4 * scale),
                      Color::dwhite_lblack(theme), pct_text);

        float value_x = pos.x + 65 * scale;
        float value_col_spacing = 90.0f * scale;
        for (int j = 0; j < 3; j++){
            char value_text[16];
            snprintf(value_text, sizeof(value_text), "%.2f", Battery_values[i][j]);

            draw->AddText(ImVec2(value_x + (value_col_spacing * j), row_y + 25 * scale),
                          Color::dwhite_lblack(theme), value_text);
            draw->AddText(ImVec2(value_x + (value_col_spacing * j) + 30 * scale, row_y + 25 * scale),
                          Color::dwhite_lblack(theme), Battery_text_value[j]);
        }

        if (i < 1) {
            draw->AddLine(
                ImVec2(pos.x + 14 * scale, row_y + row_height - 12 * scale),
                ImVec2(pos.x + 296 * scale, row_y + row_height - 12 * scale),
                Color::panelBorder(theme), 1.0f);
        }
    }

    float motors_section_y = pos.y + row_start_y + (row_height * 2);

    draw->AddLine(
        ImVec2(pos.x + 14 * scale, motors_section_y),
        ImVec2(pos.x + 296 * scale, motors_section_y),
        Color::panelBorder(theme), 3.0f);

    InfoPanels::CollapseButton(draw, ImVec2(pos.x + 290 * scale, pos.y + 10 * scale), scale, Motor_panel_open, theme);

    if (Motor_panel_open) {
        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 15 * scale, motors_section_y + 15 * scale),
                      IM_COL32(255, 140, 0, 255), "MOTOR USAGE");
        ImGui::PopFont();

        float Motor_values[4] = {battery_percentage[0], 0.0f, 0.0f, 0.0f};
        const char* Motor_labels[4] = {"M1", "M2", "M3", "M4"};
        float Motor_row_spacing = 30.0f * scale;
        float bar_x_start = 45.0f * scale;
        float bar_width   = 180.0f * scale;
        float bar_height  = 8.0f * scale;
        ImU32 motor_color = IM_COL32(255, 140, 0, 255);

        for (int i = 0; i < 4; i++){
            float row_y = motors_section_y + 45.0f * scale + (Motor_row_spacing * i);

            draw->AddText(ImVec2(pos.x + 15 * scale, row_y),
                          Color::dwhite_lblack(theme), Motor_labels[i]);

            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale),
                ImVec2(pos.x + bar_x_start + bar_width, row_y + 3 * scale + bar_height),
                Color::panelBorder(theme), 4.0f * scale, ImDrawFlags_RoundCornersAll);

            float fill_width = map_value(Motor_values[i], 0.0f, 1.0f, 0.0f, bar_width);
            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale),
                ImVec2(pos.x + bar_x_start + fill_width, row_y + 3 * scale + bar_height),
                motor_color, 4.0f * scale, ImDrawFlags_RoundCornersAll);

            char motor_text[16];
            snprintf(motor_text, sizeof(motor_text), "%.0f%%", Motor_values[i] * 100.0f);
            draw->AddText(ImVec2(pos.x + bar_x_start + bar_width + 50 * scale, row_y),
                          Color::dwhite_lblack(theme), motor_text);
        }
    }

    InfoPanels::End_panels();
}

void InfoPanels::Position_Info(float scale, bool theme, float pos_meter[3]){

    ImVec2 pos = InfoPanels::Begin_panels("PositionInfo",210 * scale,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "State-NED");
    ImGui::PopFont();

    float target_meter[3] = {2.1f, 0.3f, -1.5f};
    float vel_meter[3] = {0.0f, 0.3f, -0.23f};
    ImU32 xyz_color[3] = {
        IM_COL32(232, 45, 39, 255),
        IM_COL32(122, 193, 66, 255),
        IM_COL32(44, 169, 225, 255),
    };
    const char* xyz_text[3] = {"X", "Y", "Z"};
    const char* explain_text[3] = {"POS m", "TGT m", "Vel m/s"};
    int x_space = 75 * scale;
    int y_space = 30 * scale;
   for (int i = 0; i < 3; i++){
        draw->AddText(ImVec2(pos.x + 100 + (x_space * i) * scale, pos.y + 30 * scale),
                                  xyz_color[i], xyz_text[i]);
        ImGui::PushFont(winInit.getFont(181));
        char pos_txt[16];
        snprintf(pos_txt, sizeof(pos_txt), "%.2f", pos_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 60 * scale),
                                 Color::white_black(theme) , pos_txt);
        ImGui::PopFont();
        char tgt_txt[16];
        snprintf(tgt_txt, sizeof(tgt_txt), "%.2f", target_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 90 * scale),
        Color::dwhite_lblack(theme), tgt_txt);
        char vel_txt[16];
        snprintf(vel_txt, sizeof(vel_txt), "%.2f", vel_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale, pos.y + 120 * scale),
        Color::dwhite_lblack(theme), vel_txt);

        draw->AddText(ImVec2(pos.x + 10 * scale, pos.y + 60 * scale  + (y_space *i) ),
        Color::dwhite_lblack(theme), explain_text[i]);

    }
    draw->AddLine(
        ImVec2(pos.x + 14 * scale, pos.y + 155 * scale),
        ImVec2(pos.x + 296 * scale, pos.y + 155 * scale),
        Color::panelBorder(theme), 3.0f);

    float speed_numb = sqrt(pow(vel_meter[0], 2.0f) + pow(vel_meter[1],2.0f));
    char speed_char[8];
    snprintf(speed_char, sizeof(speed_char), "%.2f", speed_numb);
    draw->AddText(ImVec2((pos.x + 250 * scale), (pos.y + 170 * scale)), Color::dwhite_lblack(theme), speed_char);
    draw->AddText(ImVec2((pos.x + 10 * scale), (pos.y + 170 * scale)), Color::dwhite_lblack(theme), "Speed m/s");

    InfoPanels::End_panels();
}

void InfoPanels::Probe_Info(float scale, bool theme){

    ImVec2 pos = InfoPanels::Begin_panels("ProbeInfo",170,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Probe Info");

    InfoPanels::End_panels();
}