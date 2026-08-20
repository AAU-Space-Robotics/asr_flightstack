#include "info_panels.h"
#include "app_window.h"
#include "widgets.h"


float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Graphs::battery_graph(ImDrawList* draw_list, float x1, float y1, float x2, float y2,
                                                    float x1b, float y1b, float x2b, float y2b,
                            const UiScale& scale) {
    ImU32 color = IM_COL32(204, 204, 204, 255);

    draw_list->AddRect(ImVec2(x1 * scale.x, y1 * scale.y), ImVec2(x2 * scale.x, y2 * scale.y),
                        color, 1.0f * scale.uniform(), ImDrawFlags_RoundCornersAll,
                        3.0f * scale.uniform());

    draw_list->AddRect(ImVec2(x1b * scale.x, y1b * scale.y), ImVec2(x2b * scale.x, y2b * scale.y),
                        color, 1.0f * scale.uniform(), ImDrawFlags_RoundCornersAll,
                        3.0f * scale.uniform());
}

void DrawPanelBackground(ImDrawList* draw_list, ImVec2 pos, ImVec2 size,
                          ImU32 bg_color, ImU32 border_color,
                          float rounding, float border_thickness) {
    ImVec2 p_min = pos;
    ImVec2 p_max = ImVec2(pos.x + size.x, pos.y + size.y);

    draw_list->AddRectFilled(p_min, p_max, bg_color, rounding);
    draw_list->AddRect(p_min, p_max, border_color, rounding, 0, border_thickness);
}

ImVec2 InfoPanels::Panel_tracker(ImVec2 size, const UiScale& scale){

    const int max_panel_space = 1500;
    const float gap = 10.0f;
    if (cur_panel_space + size.y > max_panel_space){
        //std::cout << "Damn boy" << std::endl;
    }

    ImVec2 panel_pos = ImVec2(1580 * scale.x, (70 * scale.y + cur_panel_space));
    cur_panel_space += (size.y + gap);
    tracker.push_back({{1580, (float)cur_panel_space}, {size.x, size.y}, 0});
    return panel_pos;
}

void InfoPanels::ResetPanelTracking() {
    cur_panel_space = 0;
    tracker.clear();
}

ImVec2 InfoPanels::Begin_panels(const char* id, int y_size, const UiScale& scale, bool theme){

    const ImVec2 size = ImVec2(310 * scale.x, y_size);
    ImVec2 pos = InfoPanels::Panel_tracker(size, scale);
    ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * scale.x, 8 * scale.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale.uniform());
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

void BeginFixedPanel(const char* id, ImVec2 pos, ImVec2 size, const UiScale& scale, bool theme,
                      ImGuiWindowFlags extraFlags, ImVec2 padding, bool allow_scroll) {
    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale.x, padding.y * scale.y));
    ImGuiWindowFlags flags = extraFlags;
    if (!allow_scroll) {
        flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    }
    ImGui::BeginChild(id, size, true, flags);
}

void EndFixedPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void BeginOverlayPanel(ImDrawList* draw_list, const char* id, ImVec2 pos, ImVec2 size,
                        const UiScale& scale, bool theme,
                        ImGuiWindowFlags extraFlags, ImVec2 padding) {
    DrawPanelBackground(draw_list, pos, size,
                         ImGui::ColorConvertFloat4ToU32(Color::panelColor(theme)),
                         Color::panelBorder(theme),
                         12.0f * scale.uniform(), 2.0f * scale.uniform());

    ImGui::SetCursorPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x * scale.x, padding.y * scale.y));
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

bool InfoPanels::CollapseButton(ImDrawList* draw_list, ImVec2 pos, const UiScale& scale, bool& isOpen, bool theme){

    float size = 16.0f * scale.uniform();
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

void InfoPanels::Battery_Info(const UiScale& scale, bool theme, const BatteryState& battery_info_C, const BatteryState& battery_info_M, double motor_speeds[4]){
    int size;
    static bool Motor_panel_open = false;
    if(Motor_panel_open) {
        size = 350 * scale.y;
    } else {
        size = 190 * scale.y;
    }

    ImVec2 pos = InfoPanels::Begin_panels("BatteryInfo", size, scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Power & Motors");
    ImGui::PopFont();

    const char* Battery_row_labels[2] = {"MOTORS", "COMPUTER"};
    ImU32 Battery_row_colors[2] = { IM_COL32(255, 140, 0, 255), IM_COL32(70, 150, 255, 255) };

    // Index 0 = MOTORS (battery_info_M), index 1 = COMPUTER (battery_info_C) -- matches the labels/colors above
    BatteryState Battery_states[2] = { battery_info_M, battery_info_C };

    float Battery_values_motors[3]  = {
        static_cast<float>(battery_info_M.voltage),
        static_cast<float>(battery_info_M.discharged_mah),
        static_cast<float>(battery_info_M.average_current)
    };
    float Battery_values_compute[3] = {
        static_cast<float>(battery_info_C.voltage),
        static_cast<float>(battery_info_C.discharged_mah),
        static_cast<float>(battery_info_C.average_current)
    };
    float* Battery_values[2] = { Battery_values_motors, Battery_values_compute };

    const char* Battery_text[3] = {"Voltage","Discharge", "Avg. Current"};
    const char* Battery_text_value[3] = {"V", "mAh", "A"};

    float row_height = 70.0f * scale.y;
    float row_start_y = 45.0f * scale.y;

    for (int i = 0; i < 2; i++){
        float row_y = pos.y + row_start_y + (row_height * i);

        float charge_pct = static_cast<float>(Battery_states[i].charge_remaining);

        ImU32 battery_color;
        if (charge_pct > 0.5f) {
            battery_color = IM_COL32(0, 255, 0, 255);
        } else if (charge_pct > 0.25f) {
            battery_color = IM_COL32(255, 255, 0, 255);
        } else {
            battery_color = IM_COL32(255, 0, 0, 255);
        }

        float icon_x = pos.x + 15 * scale.x;
        float icon_top = row_y;
        float icon_bottom = row_y + 55 * scale.y;

        draw->AddRectFilled(
            ImVec2(icon_x + 9 * scale.x, icon_top - 6 * scale.y),
            ImVec2(icon_x + 21 * scale.x, icon_top),
            Battery_row_colors[i], 3.0f * scale.uniform(), ImDrawFlags_RoundCornersTop);

        float battery_progressbar = map_value(charge_pct, 0.0f, 1.0f,
                                               icon_bottom - 4.0f * scale.y,
                                               icon_top + 6.0f * scale.y);
        draw->AddRectFilled(
            ImVec2(icon_x + 3 * scale.x, battery_progressbar),
            ImVec2(icon_x + 27 * scale.x, icon_bottom - 2.0f * scale.y),
            battery_color, 3.0f * scale.uniform(), ImDrawFlags_RoundCornersBottom);

        draw->AddRect(
            ImVec2(icon_x, icon_top),
            ImVec2(icon_x + 30 * scale.x, icon_bottom),
            Battery_row_colors[i], 6.0f * scale.uniform(), ImDrawFlags_RoundCornersAll, 2.5f * scale.uniform());

        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 55 * scale.x, row_y + 2 * scale.y),
                      Battery_row_colors[i], Battery_row_labels[i]);
        ImGui::PopFont();

        char pct_text[16];
        snprintf(pct_text, sizeof(pct_text), "%.0f%%", charge_pct * 100.0f);
        draw->AddText(ImVec2(pos.x + 260 * scale.x, row_y + 4 * scale.y),
                      Color::dwhite_lblack(theme), pct_text);

        float value_x = pos.x + 65 * scale.x;
        float value_col_spacing = 90.0f * scale.x;
        for (int j = 0; j < 3; j++){
            char value_text[16];
            snprintf(value_text, sizeof(value_text), "%.2f", Battery_values[i][j]);

            draw->AddText(ImVec2(value_x + (value_col_spacing * j), row_y + 25 * scale.y),
                          Color::dwhite_lblack(theme), value_text);
            draw->AddText(ImVec2(value_x + (value_col_spacing * j) + 40 * scale.x, row_y + 25 * scale.y),
                          Color::dwhite_lblack(theme), Battery_text_value[j]);
        }

        if (i < 1) {
            draw->AddLine(
                ImVec2(pos.x + 14 * scale.x, row_y + row_height - 12 * scale.y),
                ImVec2(pos.x + 296 * scale.x, row_y + row_height - 12 * scale.y),
                Color::panelBorder(theme), 1.0f);
        }
    }

    float motors_section_y = pos.y + row_start_y + (row_height * 2);
    InfoPanels::CollapseButton(draw, ImVec2(pos.x + 270 * scale.x, pos.y + 10 * scale.y), scale, Motor_panel_open, theme);

    if (Motor_panel_open) {
        ImGui::PushFont(winInit.getFont(181));
        draw->AddText(ImVec2(pos.x + 15 * scale.x, motors_section_y + 15 * scale.y),
                      IM_COL32(255, 140, 0, 255), "MOTOR USAGE");
        ImGui::PopFont();

        const char* Motor_labels[4] = {"M1", "M2", "M3", "M4"};
        float Motor_row_spacing = 30.0f * scale.y;
        float bar_x_start = 45.0f * scale.x;
        float bar_width   = 180.0f * scale.x;
        float bar_height  = 8.0f * scale.y;
        ImU32 motor_color = IM_COL32(255, 140, 0, 255);

        for (int i = 0; i < 4; i++){
            float row_y = motors_section_y + 45.0f * scale.y + (Motor_row_spacing * i);
            float motor_val = static_cast<float>(motor_speeds[i]);

            draw->AddText(ImVec2(pos.x + 15 * scale.x, row_y),
                          Color::dwhite_lblack(theme), Motor_labels[i]);

            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale.y),
                ImVec2(pos.x + bar_x_start + bar_width, row_y + 3 * scale.y + bar_height),
                Color::panelBorder(theme), 4.0f * scale.uniform(), ImDrawFlags_RoundCornersAll);

            float fill_width = map_value(motor_val, 0.0f, 1.0f, 0.0f, bar_width);
            draw->AddRectFilled(
                ImVec2(pos.x + bar_x_start, row_y + 3 * scale.y),
                ImVec2(pos.x + bar_x_start + fill_width, row_y + 3 * scale.y + bar_height),
                motor_color, 4.0f * scale.uniform(), ImDrawFlags_RoundCornersAll);

            char motor_text[16];
            snprintf(motor_text, sizeof(motor_text), "%.0f%%", motor_val * 100.0f);
            draw->AddText(ImVec2(pos.x + bar_x_start + bar_width + 40 * scale.x, row_y),
                          Color::dwhite_lblack(theme), motor_text);
        }

        draw->AddLine(
            ImVec2(pos.x + 14 * scale.x, motors_section_y),
            ImVec2(pos.x + 296 * scale.x, motors_section_y),
            Color::panelBorder(theme), 3.0f);
    }

    InfoPanels::End_panels();
}

void InfoPanels::Position_Info(const UiScale& scale, bool theme, float pos_meter[3], float target_meter[3], float vel_meter[3]){

    ImVec2 pos = InfoPanels::Begin_panels("PositionInfo",210 * scale.y,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "State-NED");
    ImGui::PopFont();

    ImU32 xyz_color[3] = {
        IM_COL32(232, 45, 39, 255),
        IM_COL32(122, 193, 66, 255),
        IM_COL32(44, 169, 225, 255),
    };
    const char* xyz_text[3] = {"X", "Y", "Z"};
    const char* explain_text[3] = {"POS m", "TGT m", "Vel m/s"};
    int x_space = 75 * scale.x;
    int y_space = 30 * scale.y;
   for (int i = 0; i < 3; i++){
        draw->AddText(ImVec2(pos.x + 100 + (x_space * i) * scale.x, pos.y + 30 * scale.y),
                                  xyz_color[i], xyz_text[i]);
        ImGui::PushFont(winInit.getFont(181));
        char pos_txt[16];
        snprintf(pos_txt, sizeof(pos_txt), "%.2f", pos_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale.x, pos.y + 60 * scale.y),
                                 Color::white_black(theme) , pos_txt);
        ImGui::PopFont();
        char tgt_txt[16];
        snprintf(tgt_txt, sizeof(tgt_txt), "%.2f", target_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale.x, pos.y + 90 * scale.y),
        Color::dwhite_lblack(theme), tgt_txt);
        char vel_txt[16];
        snprintf(vel_txt, sizeof(vel_txt), "%.2f", vel_meter[i]);
        draw->AddText(ImVec2(pos.x + 95 + (x_space * i) * scale.x, pos.y + 120 * scale.y),
        Color::dwhite_lblack(theme), vel_txt);

        draw->AddText(ImVec2(pos.x + 10 * scale.x, pos.y + 60 * scale.y  + (y_space *i) ),
        Color::dwhite_lblack(theme), explain_text[i]);

    }
    draw->AddLine(
        ImVec2(pos.x + 14 * scale.x, pos.y + 155 * scale.y),
        ImVec2(pos.x + 296 * scale.x, pos.y + 155 * scale.y),
        Color::panelBorder(theme), 3.0f);

    float speed_numb = sqrt(pow(vel_meter[0], 2.0f) + pow(vel_meter[1],2.0f));
    char speed_char[8];
    snprintf(speed_char, sizeof(speed_char), "%.2f", speed_numb);
    draw->AddText(ImVec2((pos.x + 250 * scale.x), (pos.y + 170 * scale.y)), Color::dwhite_lblack(theme), speed_char);
    draw->AddText(ImVec2((pos.x + 10 * scale.x), (pos.y + 170 * scale.y)), Color::dwhite_lblack(theme), "Ground Speed m/s");

    InfoPanels::End_panels();
}

void InfoPanels::Probe_Info(const UiScale& scale, bool theme, const ProbeData& info){
    int size;
    static bool Probe_panel_open = true;
    if(Probe_panel_open) {
        size = 215 * scale.y;
    } else {
        size = 40 * scale.y;
    }

    ImVec2 pos = InfoPanels::Begin_panels("ProbeInfo",size,scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "Probe Info");
    ImGui::PopFont();
    InfoPanels::CollapseButton(draw, ImVec2(pos.x + 270 * scale.x, pos.y + 10 * scale.y), scale, Probe_panel_open, theme);
    if(Probe_panel_open)
    {
        int numb_probes = info.probes.size();
        int start_x = 20 * scale.x;
        int start_y = 35 * scale.y;
        int y_space = 35 * scale.y;

        for (int i = 0; i < numb_probes; i++){
            const Probe& p = info.probes[i];

            char probe_text[32];
            snprintf(probe_text, sizeof(probe_text), "Probe: %d", i + 1);
            draw->AddText(ImVec2((pos.x + start_x), (pos.y + start_y) + (y_space * i)), Color::dwhite_lblack(theme), probe_text);

            ImGui::PushFont(winInit.getFont(14));
            char values_text[64];
            snprintf(values_text, sizeof(values_text), "%.2f  %.2f  %.2f    %.2f  %.2f  %.2f",
                    p.x, p.y, p.z, p.sigma_x, p.sigma_y, p.sigma_z);
            draw->AddText(ImVec2((pos.x + start_x + 85 * scale.x), (pos.y + start_y) + (y_space * i)), Color::dwhite_lblack(theme), values_text);
            ImGui::PopFont();

            draw->AddLine(
            ImVec2(pos.x + 14 * scale.x, pos.y + start_y + 23 * scale.y + (y_space * i)),
            ImVec2(pos.x + 296 * scale.x, pos.y + start_y + 23 * scale.y + (y_space * i)),
            Color::panelBorder(theme), 3.0f);
        }

    }

    InfoPanels::End_panels();
}
bool Logs::outputlog(ImVec2 pos, const UiScale& scale, bool theme, const std::vector<LogEntry>& log_lines, const LogFilters& filters){
    ImVec2 size = ImVec2(900 * scale.x, 160 * scale.y);
    ImGui::SetCursorPos(ImVec2(pos.x * scale.x, pos.y * scale.y));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::panelColor(theme));
    ImGui::PushStyleColor(ImGuiCol_Border, Color::panelBorder(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * scale.x, 8 * scale.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale.uniform());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale.uniform());
    ImGui::BeginChild("ConsoleLoggerOverpanel", size, true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();

    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((windowPos.x + 10), (windowPos.y + 10)), Color::white_black(theme), "Console logger");
    ImGui::PopFont();
    
    ImGui::SetCursorScreenPos(ImVec2(windowPos.x + 15 * scale.x, windowPos.y + 50 * scale.y));
    ImVec2 log_area_size = ImVec2(size.x - 35 * scale.x, size.y - 65 * scale.y);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::bgColor(theme));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f * scale.uniform());
    ImGui::BeginChild("ConsoleLoggerContent", log_area_size, false,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    ImGui::PushFont(winInit.getFont(18));
    for (const auto& entry : log_lines) {
        if (entry.level == LogLevel::INFO  && !filters.show_info)  continue;
        if (entry.level == LogLevel::WARN  && !filters.show_warn)  continue;
        if (entry.level == LogLevel::ERROR && !filters.show_error) continue;
        if (entry.level == LogLevel::DEBUG && !filters.show_debug) continue;

        const char* level_text = "";
        ImVec4 color;
        switch (entry.level) {
            case LogLevel::ERROR: level_text = "ERROR"; color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break;
            case LogLevel::WARN:  level_text = "WARN";  color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); break;
            case LogLevel::INFO:  level_text = "INFO";  color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break;
            case LogLevel::DEBUG: level_text = "DEBUG"; color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break;
        }
        ImGui::Text("[%s]", entry.timestamp.c_str());
        ImGui::SameLine();
        ImGui::TextColored(color, "%s", level_text);
        ImGui::SameLine();
        ImGui::Text("%s", entry.message.c_str());
    }
    ImGui::PopFont();

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    bool clear_clicked = Widgets::DrawSmallRedButton(draw, ImVec2(windowPos.x + size.x - 70 * scale.x, windowPos.y + 30 * scale.y), scale, "clear");

    ImGui::EndChild();          
    ImGui::PopStyleVar(3);     
    ImGui::PopStyleColor(2);   
    return clear_clicked;
}

void InfoPanels::GNSS_Info(const UiScale& scale, bool theme,
                        double accuracy,
                        double accuracy_target,
                        double duration,
                        const GPSState& gps_info){
    int size;
    static bool GNSS_panel_open = true;
    if(GNSS_panel_open) {
        size = 150 * scale.y;
    } else {
        size = 40 * scale.y;
    }

    ImVec2 pos = InfoPanels::Begin_panels("GNSSInfo", size, scale, theme);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::PushFont(winInit.getFont(181));
    draw->AddText(ImVec2((pos.x + 10), (pos.y + 10)), Color::white_black(theme), "GNSS");
    ImGui::PopFont();

    InfoPanels::CollapseButton(draw, ImVec2(pos.x + 270 * scale.x, pos.y + 10 * scale.y), scale, GNSS_panel_open, theme);
    if(GNSS_panel_open)
    {
    float col_start_x = 20.0f * scale.x;
    float col_spacing = 70.0f * scale.x;

    // --- Lat/Lon, right under the title ---
    char latitude_text[32];
    snprintf(latitude_text, sizeof(latitude_text), "Lat:  %.5f", gps_info.latitude);
    draw->AddText(ImVec2(pos.x + col_start_x, pos.y + 40 * scale.y),
        Color::dwhite_lblack(theme), latitude_text);

    char longitude_text[32];
    snprintf(longitude_text, sizeof(longitude_text), "Lon:  %.5f", gps_info.longitude);
    draw->AddText(ImVec2(pos.x + col_start_x + col_spacing * 2, pos.y + 40 * scale.y),
        Color::dwhite_lblack(theme), longitude_text);


    float value_row_y = 70 * scale.y;
    float label_row_y = 85 * scale.y;

    char sat_used_text[32];
    snprintf(sat_used_text, sizeof(sat_used_text), "%d", gps_info.satellites_used);
    draw->AddText(ImVec2(pos.x + col_start_x + col_spacing * 2, pos.y + value_row_y),
        Color::dwhite_lblack(theme), sat_used_text);
    ImGui::PushFont(winInit.getFont(14));
    draw->AddText(ImVec2(pos.x + col_start_x + col_spacing * 2, pos.y + label_row_y),
        Color::dwhite_lblack(theme), "Sats Used");
    ImGui::PopFont();

    char cur_accuracy_text[32];
    snprintf(cur_accuracy_text, sizeof(cur_accuracy_text), "%.0f m", accuracy);
    draw->AddText(ImVec2(pos.x + col_start_x, pos.y + value_row_y),
        Color::dwhite_lblack(theme), cur_accuracy_text);
    ImGui::PushFont(winInit.getFont(14));
    draw->AddText(ImVec2(pos.x + col_start_x, pos.y + label_row_y),
        Color::dwhite_lblack(theme), "Accuracy");
    ImGui::PopFont();

    char duration_text[32];
    snprintf(duration_text, sizeof(duration_text), "%.0f s", duration);
    draw->AddText(ImVec2(pos.x + col_start_x + col_spacing, pos.y + value_row_y),
        Color::dwhite_lblack(theme), duration_text);
    ImGui::PushFont(winInit.getFont(14));
    draw->AddText(ImVec2(pos.x + col_start_x + col_spacing, pos.y + label_row_y),
        Color::dwhite_lblack(theme), "Duration");
    ImGui::PopFont();

    // --- Survey progress bar ---
    float bar_x_start = 20.0f * scale.x;
    float bar_start_y = 120.0f * scale.y;
    float bar_width    = 200.0f * scale.x;
    float bar_height   = 8.0f * scale.y;
    ImU32 survey_color = IM_COL32(255, 140, 0, 255);
    ImU32 overshoot_color = IM_COL32(0, 220, 100, 255);

    bool target_reached = (accuracy_target > 0.0 && accuracy <= accuracy_target);

    float fill_width;
    float overshoot_x = -1.0f;

    if (!target_reached) {
        float progress = (accuracy_target > 0.0)
            ? static_cast<float>(accuracy_target / accuracy)
            : 0.0f;
        progress = std::clamp(progress, 0.0f, 1.0f);
        fill_width = progress * bar_width;
    } else {
        fill_width = bar_width;
        float overshoot_frac = 1.0f - static_cast<float>(accuracy / accuracy_target);
        overshoot_frac = std::clamp(overshoot_frac, 0.0f, 1.0f);
        overshoot_x = bar_x_start + overshoot_frac * bar_width;
    }

    draw->AddRectFilled(
        ImVec2(pos.x + bar_x_start, pos.y + bar_start_y),
        ImVec2(pos.x + bar_x_start + bar_width, pos.y + bar_start_y + bar_height),
        Color::panelBorder(theme), 4.0f * scale.uniform(), ImDrawFlags_RoundCornersAll);

    draw->AddRectFilled(
        ImVec2(pos.x + bar_x_start, pos.y + bar_start_y),
        ImVec2(pos.x + bar_x_start + fill_width, pos.y + bar_start_y + bar_height),
        survey_color, 4.0f * scale.uniform(), ImDrawFlags_RoundCornersAll);

    if (overshoot_x >= 0.0f) {
        draw->AddLine(
            ImVec2(pos.x + overshoot_x, pos.y + bar_start_y - 2.0f * scale.y),
            ImVec2(pos.x + overshoot_x, pos.y + bar_start_y + bar_height + 2.0f * scale.y),
            overshoot_color, 2.0f * scale.uniform());
    }

    // --- Accuracy percentage -- aligned to the bar's row, same convention as motor % text ---
    double pct = (accuracy > 0.0) ? (accuracy_target / accuracy) * 100.0 : 0.0;
    char accuracy_pct_text[32];
    snprintf(accuracy_pct_text, sizeof(accuracy_pct_text), "%.0f%%", pct);
    draw->AddText(ImVec2(pos.x + bar_x_start + bar_width + 50 * scale.x, pos.y + bar_start_y - 8 * scale.y),
        Color::dwhite_lblack(theme), accuracy_pct_text);
    }
    InfoPanels::End_panels();
}