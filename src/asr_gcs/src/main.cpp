#include <iostream>
#include "interfaceutils.h"
#include "statemanager.h"
#include "app_window.h"
#include "info_panels.h"
#include "map.h"
#include "widgets.h"
#include <algorithm>
#include <opencv2/core/utils/logger.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <thread>

WindowInitializer winInit;
Widgets widgets;
Location location;
Color colors;
TestFunc test_functions;
InfoPanels info_panels;

using namespace std;


int main(int argc, char **argv) {
    //rclcpp::init(argc, argv);
    //auto node = std::make_shared<LabBaseNode>(); //!!!! FOR THE ROS LATER

    //thread ros_thread([node]() {
    //    rclcpp::spin(node);
    //});    
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    bool armButton = false;
    bool theme = 1;
    int map_zoom = 20;
    bool arming_state = false;
    int panel = 0;
    
    DroneInformation Info;

    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    });

    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Find screen resolution
    winInit.GetPrimaryMonitorResolution(windowVar::monitor_w, windowVar::monitor_h);
    windowVar::display_w = windowVar::monitor_w;
    windowVar::display_h = windowVar::monitor_h;

    winInit.Setup();
    GLFWwindow* window = glfwCreateWindow(windowVar::display_w, windowVar::display_h, "AAU SPACE ROBOTICS CONTROL STATION", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    winInit.loadFonts(); // Load fonts once
    
     // ------ Image for buttons
    string package_path = ament_index_cpp::get_package_share_directory("asr_gcs");
    string path = package_path + "/images/";

    vector<pair<string, string>> image_files = {
        {"sun" , "sun.png"},
        {"moon", "moon.png"},
        {"plus" , "plus.png"},
        {"plus_white", "plus_white.png"},
        {"minus" , "minus.png"},
        {"minus_white", "minus_white.png"},
        {"aau_logo" , "AAU_Space_Robotics_Logo.png"},
        {"up",       "up.png"},
        {"up_white", "up_white.png"},
        {"down",    "down.png"},
        {"down_white", "down_white.png"},
        {"home",    "home.png"},
        {"home_white", "home_white.png"},
        {"origin",      "origin.png"},
        {"origin_white", "origin_white.png"},
        {"job",        "job.png"},
        {"job_white",        "job_white.png"},
        {"f_mode",      "f_mode.png"},
        {"f_mode_white",      "f_mode_white.png"}
    };
    
    GLuint placeholderTile = location.display_map((package_path + "/images/tile_placeholder.png").c_str(), 1.0f);

    unordered_map<string, GLuint> images;
    for (const auto& [key, filename] : image_files){
        string full_path = path + filename;
        images[key] = widgets.LoadButtonImage(full_path.c_str());
    }


    // Register the callback with GLFW
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Set size, color, font and windows - - - - - - - - - - - - - - - - - - - - - - 
        ImVec4 bg = colors.bgColor(theme);
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);
       

        //Update
        const int screen_width = 1920; // Reference screen width
        const int screen_height = 1080; // Reference screen height
        int x_sc, y_sc;
        glfwGetWindowSize(window, &x_sc, &y_sc);
        float scale = std::max(static_cast<float>(x_sc) / screen_width, 
                               static_cast<float>(y_sc) / screen_height);
        
        ImGui::GetIO().FontGlobalScale = scale;
        // Set the GLFW window size
        //winInit.UpdateWindowSize(scale);
        //glfwSetWindowSize(window, windowVar::display_w, windowVar::display_h);

        windowVar::display_w = x_sc;
        windowVar::display_h = y_sc;

        // ---------- Start of Interface ------------------
        
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowVar::display_w, (float)windowVar::display_h));
        ImGui::Begin("GCS Interface", nullptr,
                     ImGuiWindowFlags_NoScrollbar|
                     ImGuiWindowFlags_NoScrollWithMouse | 
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);  
         // --- ------------------------------------Side panel--------------------------------------------

        BeginFixedPanel("SidePanel", ImVec2(-20 * scale, -80 * scale), ImVec2(80 * scale, y_sc + 1000 * scale),
                scale, theme, 0, ImVec2(0, 0));
   
        GLuint Day_Night_Icon = theme ? images.at("sun") : images.at("moon");

        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 980 * scale),"Day Night",scale, Day_Night_Icon, theme, 0, 0)) {
            theme = !theme;
            
        }
        GLuint Flight_icon = theme ? images.at("f_mode_white") : images.at("f_mode");
        if (panel == 0) {
            ImVec2 highlight_center = ImVec2(30 * scale, 100 * scale);
            float highlight_size = (26 + 2) * scale;  // match the button's own size_x/size_y (but_size=0, img_size=2 here)
            draw_list->AddRectFilled(
                ImVec2(highlight_center.x - highlight_size, highlight_center.y - highlight_size),
                ImVec2(highlight_center.x + highlight_size, highlight_center.y + highlight_size),
                IM_COL32(255, 130, 30, 255),
                12.0f  // match rounding used inside CustomButton
            );
        }
        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 100 * scale),"FlightMode",scale, Flight_icon, theme, 0, 2)) {
            panel = 0;
            
        }
        GLuint Job_Icon = theme ? images.at("job_white") : images.at("job");

        if (panel == 1) {
            ImVec2 highlight_center = ImVec2(30 * scale, 160 * scale);
            float highlight_size = (26 + 2) * scale;
            draw_list->AddRectFilled(
                ImVec2(highlight_center.x - highlight_size, highlight_center.y - highlight_size),
                ImVec2(highlight_center.x + highlight_size, highlight_center.y + highlight_size),
                IM_COL32(255, 130, 30, 255),
                12.0f
            );
        }
        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 160 * scale),"Job",scale, Job_Icon, theme, 0, 2)) {
            panel = 1;
            
        }


        EndFixedPanel();

        
        // ----------------------------------------Top Panel-----------------------------------------
        BeginFixedPanel("TopPanel", ImVec2(-20 * scale, -20 * scale), ImVec2(x_sc + 1000 * scale, 80 * scale),
                scale, theme, 0, ImVec2(0, 0));
        
        draw_list->AddImageRounded(
            (ImTextureID)(intptr_t)images.at("aau_logo"),
            ImVec2(5 * scale, 5 * scale),  
            ImVec2(55 * scale, 55 * scale),  // inset max
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 200),
            12.0f
        );
        for (int i = 0; i <= 200; i+=200){
            draw_list->AddLine(
                ImVec2((60 + i) * scale, 10 * scale),
                ImVec2((60 + i) * scale, 45 * scale),   
                colors.panelBorder(theme),
                1.0f
            );
        }
        if (widgets.ArmButton(draw_list,ImVec2(340 * scale, 28 * scale), scale, theme, arming_state )){
            arming_state = !arming_state;
        }

        
        EndFixedPanel(); 

        switch (panel)
        {

        case 0:
        {
        //--------------------------MAP--------------------------------------------------------------
        static double testLat = 57.063f, testLon = 10.032f; //! Remeber to remove
        BeginFixedPanel("MapPanel", ImVec2(70 * scale, 70 * scale), ImVec2(1500 * scale, 800 * scale),
                scale, theme, 0, ImVec2(0, 0));
        location.MapWidget(testLat, testLon, 1500 * scale, 900 * scale, scale, map_zoom, placeholderTile, theme);


        if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
            std::cout << "ESTOP Button Clicked!" << std::endl;
        }
        
        ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
        widgets.AltitudeTape(1, Info.xyz_pos[2], 0.5f, theme); 

        widgets.GyroScopeIndicator(draw_list,
                                ImVec2(1320 * scale, 810 * scale),
                                Info.orientation, 
                                theme);

        widgets.Compas(draw_list,
                                ImVec2(1440 * scale, 810 * scale),
                                Info.orientation, 
                                theme);                   

        EndFixedPanel();

        //----------------------------------- Map utils panel-----------------------------------
        BeginOverlayPanel(draw_list, "MapUtilsPanel", ImVec2(1500 * scale, 72 * scale), ImVec2(49 * scale, 150 * scale), scale, theme);

        
        GLuint Plus_Icon = theme ? images.at("plus_white") : images.at("plus");
        if (widgets.CustomButton(draw_list, ImVec2(1524 * scale, 97 * scale), "Plus", scale, Plus_Icon, theme, -5, -3)) {
            if (map_zoom < 20) {
                map_zoom += 1;
            }
        }
        GLuint Minus_Icon = theme ? images.at("minus_white") : images.at("minus");
        if (widgets.CustomButton(draw_list, ImVec2(1524 * scale, 141 * scale), "Minus", scale, Minus_Icon, theme, -5, -3)) {
            if (map_zoom > 13) {
                map_zoom -= 1;
            }
        }

        EndOverlayPanel();

         // -----------------------------------Control Panel-----------------------------

        BeginOverlayPanel(draw_list, "ControlPanel", ImVec2(90 * scale, 305 * scale), ImVec2(60 * scale, 300 * scale), scale, theme);

        
        GLuint Up_Icon = theme ? images.at("up_white") : images.at("up");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 335 * scale), "Up", scale, Up_Icon, theme, -1, 5)) {
            std::cout << "Takeoff Button Clicked!" << std::endl;
        }
        ImGui::PushFont(winInit.getFont(14));
        draw_list->AddText(ImVec2(100 * scale, 362 * scale), colors.white_black(theme), "TakeOff");

        GLuint Down_Icon = theme ? images.at("down_white") : images.at("down");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 405 * scale), "Down", scale, Down_Icon, theme, -1, 5)) {
            std::cout << "Land Button Clicked!" << std::endl;
        }
        draw_list->AddText(ImVec2(107 * scale, 432 * scale), colors.white_black(theme), "Land");

        GLuint Home_Icon = theme ? images.at("home_white") : images.at("home");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 475 * scale), "Home", scale, Home_Icon, theme, -1, 5)) {
            std::cout << "Home Button Clicked!" << std::endl;
        }
        draw_list->AddText(ImVec2(106 * scale, 502 * scale), colors.white_black(theme), "Home");

        GLuint Origin_Icon = theme ? images.at("origin_white") : images.at("origin");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 545 * scale), "Origin", scale, Origin_Icon, theme, -1, 5)) {
            std::cout << "Orgigin Button Clicked!" << std::endl;
        }
        draw_list->AddText(ImVec2(106 * scale, 572 * scale), colors.white_black(theme), "Origin");

        ImGui::PopFont();
        EndOverlayPanel();
        

        // ---------For testing — replace with ROS later //!!!!!
        ImGui::SetCursorPos(ImVec2(900 * scale, 500 * scale));
        ImGui::BeginChild("TestPanel", ImVec2(600 * scale, 600 * scale), 
                       false,  // border
                       ImGuiWindowFlags_NoBackground |
                       ImGuiWindowFlags_NoScrollWithMouse | 
                       ImGuiWindowFlags_NoScrollbar);
    
        
        ImGui::InputDouble("Lat", &testLat, 0.000001, 0.01, "%.7f");
        ImGui::InputDouble("Lon", &testLon, 0.000001, 0.01, "%.7f");
        ImGui::SliderFloat("Altitude", &Info.xyz_pos[2], -20.0f, 20.0f);
        ImGui::SliderFloat("Yaw", &Info.orientation.yaw, -180.0f, 180.0f);
        ImGui::SliderFloat("Roll", &Info.orientation.roll, -180.0f, 180.0f);
        ImGui::SliderFloat("Pitch", &Info.orientation.pitch, -180.0f, 180.0f);
        ImGui::SliderFloat("BatteryVoltage", &Info.battery_values_C[0], 0, 1.0f);



        ImGui::EndChild();
        

       

        //--------------------------------------Information panels---------------------------------------
        info_panels.Battery_Info(scale, theme, Info.battery_values_C);
        info_panels.Position_Info(scale, theme);
        info_panels.Probe_Info(scale, theme);
        
        
        info_panels.ResetPanelTracking();
        
  

        }
        case 1:
        {
            
        }
        default:
            break;
        }
        ImGui::End();


        // Rendering
        ImGui::Render();
        glViewport(0, 0, windowVar::display_w, windowVar::display_h);
       
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    //rclcpp::shutdown();  
    //ros_thread.join(); 

    return 0;
}