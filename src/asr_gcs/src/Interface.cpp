
#include <iostream>
#include "interfaceutils.h"
#include <algorithm>


WindowInitializer winInit;
Widgets widgets;
Location location;
Color colors;
TestFunc test_functions;


int main(int argc, char **argv) {
    float armButton = false;
    float value = 0.0f;
    bool theme = 1;
    ImU32 armColor = IM_COL32(26, 204, 26, 255); // Green color //!!! CLEAN UP
    const char* armText = "Arm";
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
    GLFWwindow* window = glfwCreateWindow(windowVar::display_w, windowVar::display_h, "Thyra", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    winInit.loadFonts(); // Load fonts once
    
     // ------ Image for buttons
    std::string path = "/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/images/";
    std::string path_takeoff = path + "takeoff.png";
    std::string path_armed = path + "armed.png";
    GLuint image_takeoff = widgets.LoadButtonImage(path_takeoff.c_str());
    GLuint image_armed = widgets.LoadButtonImage(path_armed.c_str());

    GLuint placeholderTile = location.display_map("/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/images/tile_placeholder.png", 1.0f);


    // Register the callback with GLFW
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    fprintf(stderr, "GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, "GL_VERSION: %s\n", glGetString(GL_VERSION));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //std::cout << "Test loop running..1" << std::endl;
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Draw multi-color background
        //winInit.DrawMultiColor();
        // Set size, color, font and windows - - - - - - - - - - - - - - - - - - - - - - 
        
        ImVec4 bg = colors.bgColor(theme);
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);
       



        //Update
        const int screen_width = 1920; // Reference screen width
        const int screen_height = 1080; // Reference screen height
        int x_sc, y_sc;
        glfwGetWindowSize(window, &x_sc, &y_sc);
        float scale = std::max(static_cast<float>(x_sc) / screen_width, static_cast<float>(y_sc) / screen_height);
        
        ImGui::GetIO().FontGlobalScale = scale;
        // Set the GLFW window size
        winInit.UpdateWindowSize(scale);
        glfwSetWindowSize(window, windowVar::display_w, windowVar::display_h);
        //std::cout << "Window size set to: " << windowVar::display_w << "x" << windowVar::display_h << std::endl;

        // interface area - - - - - - - - - - - - - - - - - - - - - - - 
        
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowVar::display_w, (float)windowVar::display_h));
        ImGui::Begin("GCS Interface", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);   

        // Example content
       
        //if (armButton) {
        //    armColor = IM_COL32(204, 26, 26, 255); // Red color
        //    armText = "Disarm";
        //} else {
        //    armColor = IM_COL32(26, 204, 26, 255); // Green color
        //    armText = "Arm";
        //}
        //if (widgets.costum_square_button(armText, ImVec2(800 * scale, 50*scale), ImVec2(150 * scale, 50 * scale), winInit.getFont(28), 28.0f * scale, armColor)) {
//        //    armButton = !armButton; // Toggle button state
        //    printf("Arm button clicked. New state: %s\n", armButton ? "Armed" : "Disarmed");
        //}
        // ------------ Buttons-------------
        if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(40), 1.0f, ImVec2(1500 * scale, 70 * scale), 60.0f * scale, "ESTOP", 40.0f * scale)) {
            std::cout << "ESTOP Button Clicked!" << std::endl;
        }
        if (widgets.CustomButton(draw_list, ImVec2(1700 * scale, 100 * scale),"ARM",scale, image_takeoff)) {
            std::cout << "Takeoff pressed!" << std::endl;
            theme = 1;
        }
        if (widgets.CustomButton(draw_list, ImVec2(1800 * scale, 100 * scale),"ARM",scale, image_armed)) {
            std::cout << "Armed pressed!" << std::endl;
            theme = 0;
        }

        //ImGui::PushFont(ImGui::GetFont());
        //ImGui::SetWindowFontScale(3.0f); // 150% text size

        

        //ImGui::SetCursorPos(ImVec2(100 * scale, 100 * scale));

        // ---------For testing — replace with ROS later
        static float testLat = 57.063f, testLon = 10.032f;
        ImGui::SetCursorPos(ImVec2(10 * scale, 50 * scale));
        ImGui::InputFloat("Lat", &testLat, 0.0001f, 0.01f, "%.5f");
        ImGui::InputFloat("Lon", &testLon, 0.0001f, 0.01f, "%.5f");
        ImGui::SliderFloat("Altitude", &value, -20.0f, 20.0f);


        //-------------------MAP-----------------
        ImGui::SetCursorPos(ImVec2(0 * scale, 150 * scale));
        location.MapWidget(testLat, testLon, 1500 * scale, 900 * scale, scale, 20, placeholderTile);


        
        //-------------ALTITUDE  TAPES------------
       

        
        ImGui::SetCursorPos(ImVec2(1750 * scale, 500 * scale));
        AltitudeTape(1, value, 300.0f * scale, 0.5f, theme);  // BeginChild goes here

        ImGui::SetCursorPos(ImVec2(1600 * scale, 900 * scale));
        AltitudeTape(2, value, 300.0f * scale, 0.5f, theme);
            

        

        
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

  

    return 0;
}