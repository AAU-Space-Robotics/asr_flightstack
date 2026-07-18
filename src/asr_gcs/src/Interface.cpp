
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
    std::string path_sun = path + "sun.png";
    std::string path_moon = path + "moon.png";
    GLuint image_sun = widgets.LoadButtonImage(path_sun.c_str());
    GLuint image_moon = widgets.LoadButtonImage(path_moon.c_str());

    GLuint placeholderTile = location.display_map("/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/images/tile_placeholder.png", 1.0f);


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
        //if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(40), 1.0f, ImVec2(1500 * scale, 70 * scale), 60.0f * scale, "ESTOP", 40.0f * scale)) {
        //    std::cout << "ESTOP Button Clicked!" << std::endl;
        //}
        //if (widgets.CustomButton(draw_list, ImVec2(1700 * scale, 100 * scale),"ARM",scale, image_takeoff)) {
        //    std::cout << "Takeoff pressed!" << std::endl;
        //    theme = 1;
        //}
        //if (widgets.CustomButton(draw_list, ImVec2(1800 * scale, 100 * scale),"ARM",scale, image_armed)) {
        //    std::cout << "Armed pressed!" << std::endl;
        //    theme = 0;
        //}

        //ImGui::PushFont(ImGui::GetFont());
        //ImGui::SetWindowFontScale(3.0f); // 150% text size

        



           //-------------------MAP-----------------
        static double testLat = 57.063f, testLon = 10.032f; //! Remeber to remove
        ImGui::SetCursorPos(ImVec2(70 * scale, 70 * scale));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.panelColor(theme));
        ImGui::PushStyleColor(ImGuiCol_Border, colors.panelBorder(theme) ); 
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale); 
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * scale); 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); 
        ImGui::BeginChild("MapPanel", ImVec2(1500 * scale, 800 *  scale), 
                       true,  // border
                       ImGuiWindowFlags_NoScrollbar);
        location.MapWidget(testLat, testLon, 1500 * scale, 900 * scale, scale, 20, placeholderTile, theme);


        if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
            std::cout << "ESTOP Button Clicked!" << std::endl;
        }

        ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
        widgets.AltitudeTape(1, value, 0.5f, theme); 
        

        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);


        // ---------For testing — replace with ROS later
        ImGui::SetCursorPos(ImVec2(900 * scale, 500 * scale));
        ImGui::BeginChild("TestPanel", ImVec2(600 * scale, 600 * scale), 
                       false,  // border
                       ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    
        
        ImGui::InputDouble("Lat", &testLat, 0.000001, 0.01, "%.7f");
        ImGui::InputDouble("Lon", &testLon, 0.000001, 0.01, "%.7f");
        ImGui::SliderFloat("Altitude", &value, -20.0f, 20.0f);
    
        ImGui::EndChild();
        

        // --- ---------------Side panel---------------- //! Make hidable later maybe
        ImGui::SetCursorPos(ImVec2(-20 * scale, -80 * scale));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.panelColor(theme));
        ImGui::PushStyleColor(ImGuiCol_Border, colors.panelBorder(theme) );  // custom border color
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale); 
        ImGui::BeginChild("SidePanel", ImVec2(80 * scale, y_sc + 1000 *  scale), 
                       true,  // border
                       ImGuiWindowFlags_NoScrollbar);
        GLuint currentIcon = theme ? image_sun : image_moon;

        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 980 * scale),"Day Night",scale, currentIcon, theme)) {
            theme = !theme;
            
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        
        // ----------------Top Panel------------------:
        ImGui::SetCursorPos(ImVec2(-20 * scale, -20 * scale));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.panelColor(theme));
        ImGui::PushStyleColor(ImGuiCol_Border, colors.panelBorder(theme) );  // custom border color
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f * scale); 
        ImGui::BeginChild("TopPanel", ImVec2(x_sc + 1000 * scale, 80 * scale), 
                       true,  // border
                       ImGuiWindowFlags_NoScrollbar);
        

        
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        
        
   
        
        
       










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