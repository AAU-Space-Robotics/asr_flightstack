#include <iostream>
#include "interfaceutils.h"
#include "transformations.h"
#include "state_manager.h"
#include "app_window.h"
#include "info_panels.h"
#include "map.h"
#include "widgets.h"
#include "planner.h"
#include "planner_panel.h"
#include "height_chart.h"
#include <implot.h>


#include <algorithm>
#include <opencv2/core/utils/logger.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <thread>
#include <cstdlib>
#include <atomic>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <rcutils/logging.h>
#include <asr_comms/action/uav_command.hpp>
#include <asr_comms/msg/telemetry_battery.hpp>
#include <asr_comms/msg/telemetry_gps.hpp>
#include "asr_comms/msg/telemetry_position.hpp"
#include "asr_comms/msg/telemetry_attitude.hpp"
#include "asr_comms/msg/telemetry_battery.hpp"
#include "asr_comms/msg/telemetry_gps.hpp"
#include "asr_comms/msg/telemetry_status.hpp"
#include "asr_comms/msg/gcs_heartbeat.hpp"
#include "asr_comms/msg/uav_command.hpp"
#include "asr_comms/msg/command_ack.hpp"
#include "asr_comms/msg/servo_command.hpp"
#include "asr_comms/msg/probe_locations.hpp"
#include "asr_comms/msg/gcs_heartbeat.hpp"
#include "px4_msgs/msg/sensor_gps.hpp"
#include <px4_msgs/msg/vehicle_attitude.hpp>   
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/distance_sensor.hpp>

         

WindowInitializer winInit;
Widgets widgets;
Location location;
Color colors;
TestFunc test_functions;
InfoPanels info_panels;

using namespace std;
using namespace px4_msgs::msg;

constexpr size_t BATTERY_MAIN = 0;
constexpr size_t BATTERY_COMPUTE = 1;

using GcsHeartbeat = asr_comms::msg::GcsHeartbeat_<std::allocator<void>>;

class AAUGrouncontrol : public rclcpp::Node
{
public:
    ~AAUGrouncontrol()
    {
        if (execute_thread_.joinable()) {
            execute_thread_.join();
        }
    }
    AAUGrouncontrol()
    : Node("aau_groundcontrol_node")
    {
        rclcpp::QoS qos(10);
        qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
        qos.durability(rclcpp::DurabilityPolicy::TransientLocal);
        bool use_sim_time = false;
        clock_ = std::make_shared<rclcpp::Clock>(use_sim_time ? RCL_ROS_TIME : RCL_SYSTEM_TIME);

        std::cout << "\n"
          << "=============================\n"
          << "    * GCS STARTING... *\n"
          << "=============================\n"
          << std::endl;
        //------------------------Subscribtions---------------
        attitude_sub_ = create_subscription<VehicleAttitude>(
            "/fmu/out/vehicle_attitude", qos,
            [this](const VehicleAttitude::SharedPtr msg)
            { attitudeCallback(msg); });
        local_position_sub_ = create_subscription<VehicleLocalPosition>(
            "/fmu/out/vehicle_local_position", qos,
            [this](const VehicleLocalPosition::SharedPtr msg) { localPositionCallback(msg); });
        distance_sensor_sub_ = create_subscription<DistanceSensor>(
            "/fmu/out/distance_sensor", qos,
            [this](const DistanceSensor::SharedPtr msg) { distanceSensorCallback(msg); });
        battery_sub_main_ = create_subscription<asr_comms::msg::TelemetryBattery>(
        "telemetry/battery_main", 10,
        [this](const asr_comms::msg::TelemetryBattery::SharedPtr msg)
        { batteryCallbackmain(msg); });
        battery_sub_compute_ = create_subscription<asr_comms::msg::TelemetryBattery>(
            "telemetry/battery_compute", 10,
            [this](const asr_comms::msg::TelemetryBattery::SharedPtr msg) { batteryCallbackcompute(msg); });
        gps_sub_ = create_subscription<SensorGps>(
            "/fmu/out/vehicle_gps_position", qos,
            [this](const SensorGps::SharedPtr msg)
            { gpsCallback(msg); });
            
        heartbeat_pub_ = create_publisher<GcsHeartbeat>("in/gcs_heartbeat", qos);
        heartbeat_timer_ = create_wall_timer(
            std::chrono::milliseconds(500),
            [this]() { heartbeat(); }
        );
    }
    void start()
    {
        execute_thread_ = std::thread([this]() {
            rclcpp::spin(this->get_node_base_interface());
        });
    }
    rclcpp::Time get_time() const {
        return clock_->now();
    }
    StateManager& getStateManager() { return state_manager_; } 
private:
    StateManager state_manager_;
    std::thread execute_thread_;
    rclcpp::Subscription<VehicleAttitude>::SharedPtr attitude_sub_;
    rclcpp::Subscription<VehicleLocalPosition>::SharedPtr local_position_sub_;
    std::shared_ptr<rclcpp::Clock> clock_;
    rclcpp::Subscription<DistanceSensor>::SharedPtr distance_sensor_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_compute_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_main_;
    rclcpp::Subscription<SensorGps>::SharedPtr gps_sub_;
    rclcpp::Publisher<GcsHeartbeat>::SharedPtr heartbeat_pub_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;


    void attitudeCallback(const VehicleAttitude::SharedPtr msg)
    {
        if (!std::isfinite(msg->q[0]) || !std::isfinite(msg->q[1]) ||
            !std::isfinite(msg->q[2]) || !std::isfinite(msg->q[3])) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
                "Rejecting non-finite attitude quaternion");
            return;
        }
        StampedQuaternion attitude(get_time(), Eigen::Quaterniond(msg->q[0], msg->q[1], msg->q[2], msg->q[3]));
        state_manager_.setAttitude(attitude);
        
    }
    void localPositionCallback(const VehicleLocalPosition::SharedPtr msg)
    {
        // Reject invalid/non-finite EKF output. Dropping the message leaves the stored
        // position timestamp stale, which engages the staleness failsafe instead of
        // feeding garbage (or NaN) into the controller.
      
        if (!msg->xy_valid || !msg->z_valid ||
            !std::isfinite(msg->x) || !std::isfinite(msg->y) || !std::isfinite(msg->z)) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
                "Rejecting invalid local position (xy_valid=%d z_valid=%d)",
                msg->xy_valid, msg->z_valid);
            return;
        }

        // Note that local position refers to coordinates being expressed in cartesian coordinates from some origin point.
        Stamped3DVector origin = state_manager_.getOrigin();
        Stamped3DVector local_position(get_time(), msg->x - origin.x(), msg->y - origin.y(), msg->z - origin.z());
        state_manager_.setGlobalPosition(local_position);

        // Velocity: only trust it when the EKF marks it valid and finite. A zero velocity
        // estimate is safe for the controller; a NaN is not.
        if (msg->v_xy_valid && msg->v_z_valid &&
            std::isfinite(msg->vx) && std::isfinite(msg->vy) && std::isfinite(msg->vz)) {
            state_manager_.setGlobalVelocity(Stamped3DVector(get_time(), msg->vx, msg->vy, msg->vz));
        } else {
            state_manager_.setGlobalVelocity(Stamped3DVector(get_time(), 0.0, 0.0, 0.0));
        }

        // Acceleration has no dedicated validity flag; guard against non-finite values only.
        if (std::isfinite(msg->ax) && std::isfinite(msg->ay) && std::isfinite(msg->az)) {
            state_manager_.setGlobalAcceleration(Stamped3DVector(get_time(), msg->ax, msg->ay, msg->az));
        }
    }
    void distanceSensorCallback(const DistanceSensor::SharedPtr msg)
    {
        // your logic here — e.g.:
        // state_manager_.setDistance(msg->current_distance);
    }
    void batteryCallbackmain(const asr_comms::msg::TelemetryBattery::SharedPtr msg)
    {
        BatteryState battery;
        battery.timestamp = rclcpp::Time(msg->timestamp);   // see note below re: type conversion
        battery.voltage = msg->voltage;
        battery.charge_remaining = msg->percentage;          // using percentage here — see note below
        battery.average_current = msg->average_current;
        battery.connected = true;                            // no explicit field — inferred from message arriving at all
        // battery.cell_count left at default (0) — no matching field in TelemetryBattery

        state_manager_.setBatteryState(battery, BATTERY_MAIN);
    }
    void batteryCallbackcompute(const asr_comms::msg::TelemetryBattery::SharedPtr msg)
    {
        BatteryState battery;
        battery.timestamp = rclcpp::Time(msg->timestamp);
        battery.voltage = msg->voltage;
        battery.charge_remaining = msg->percentage;
        battery.average_current = msg->average_current;
        battery.connected = true;

        state_manager_.setBatteryState(battery, BATTERY_COMPUTE);
    }
    void gpsCallback(const SensorGps::SharedPtr msg)
    {
        GPSState gps_state;
        gps_state.latitude = msg->latitude_deg;
        gps_state.longitude = msg->longitude_deg;
        gps_state.satellites_used = msg->satellites_used;
        state_manager_.setGPSState(gps_state);

    }
    void heartbeat()
    {
        auto msg = GcsHeartbeat();
        msg.timestamp = static_cast<double>(get_time().nanoseconds()) / 1e9;
        msg.gcs_nominal = 1;

        heartbeat_pub_->publish(msg);
    }

};
EulerAngles quaternionToEulerForDisplay(const Eigen::Quaterniond& q) //trying to do a fix..........
{
    double qw = q.w(), qx = q.x(), qy = q.y(), qz = q.z();

    // Roll (x-axis rotation)
    double sinr_cosp = 2.0 * (qw * qx + qy * qz);
    double cosr_cosp = 1.0 - 2.0 * (qx * qx + qy * qy);
    double roll = std::atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation) — clamped to avoid NaN from float rounding
    double sinp = 2.0 * (qw * qy - qz * qx);
    sinp = std::clamp(sinp, -1.0, 1.0);
    double pitch = std::asin(sinp);

    // Yaw (z-axis rotation)
    double siny_cosp = 2.0 * (qw * qz + qx * qy);
    double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
    double yaw = std::atan2(siny_cosp, cosy_cosp);

    // Convert radians -> degrees to match GyroScopeIndicator's expected units
    return EulerAngles{
        roll  * (180.0 / M_PI),
        pitch * (180.0 / M_PI),
        yaw   * (180.0 / M_PI)
    };
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto ground_control = std::make_shared<AAUGrouncontrol>();
    ground_control->start();

    Planner planner(ground_control.get());
    PlannerPanel planner_panel(planner);
    DroneInformation Info;
    Transformations transformations_;

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    bool armButton = false;
    bool theme = 1;
    int map_zoom = 20;
    double Latitude = 57.063, Longitude = 10.032;
      // shared between FlightMode and Mission Planner map views
    bool arming_state = false;
    int panel = 0;
    //bool is_manual = (Info.flight_mode == FlightMode::MANUAL_AIDED); for later:
    bool is_manual = 0;
    bool sat_map = 0;

    std::atomic<bool> tiles_downloading{false};
    bool tiles_download_triggered = false;
    
    //std::thread([]() {
    //    std::system("ros2 run asr_gcs download_tiles.py");
    //}).detach();

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
    glfwSetWindowSizeLimits(window, 700, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(1); // Enable vsync
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImPlot::CreateContext();  // linked since the start of this project, never actually initialized until now
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
        {"f_mode_white",      "f_mode_white.png"},
        {"drone",       "droneImage.png"}  // placeholder icon, no light-theme variant yet
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
        if (x_sc == 0 || y_sc == 0) {
            continue;
        }

        float scale = max(static_cast<float>(x_sc) / screen_width, 
                               static_cast<float>(y_sc) / screen_height);
        scale = max(scale, 0.01f); 
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


        const StampedQuaternion& attitude = ground_control->getStateManager().getAttitude();
        Info.orientation = quaternionToEulerForDisplay(attitude.quaternion()); 
        const Stamped3DVector& position = ground_control->getStateManager().getGlobalPosition();
        Info.xyz_pos[0] = static_cast<float>(position.x());
        Info.xyz_pos[1] = static_cast<float>(position.y());
        Info.xyz_pos[2] = static_cast<float>(position.z());
        Info.gps_status = ground_control->getStateManager().getGPSState();

        bool gps_looks_valid = (Info.gps_status.satellites_used >= 4) &&
                       (Info.gps_status.latitude != 0.0) &&
                       (Info.gps_status.longitude != 0.0);
        
        if (gps_looks_valid && !tiles_download_triggered) {
            tiles_download_triggered = true;
            double lat = Info.gps_status.latitude;
            double lon = Info.gps_status.longitude;

            thread([lat, lon, &tiles_downloading]() {
                tiles_downloading = true;
                string cmd = "ros2 run asr_gcs download_tiles.py " +
                    to_string(lat) + " " + to_string(lon);
                system(cmd.c_str());
                tiles_downloading = false;
            }).detach();
        }
        //string infolat = "Long and Lat: " + to_string(Info.gps_status.latitude) + " " + to_string(Info.gps_status.longitude);
        //std::cout << infolat << std::endl;
        if (tiles_downloading.load()) {
            ImVec2 loading_pos = ImVec2(x_sc / 2.0f - 100 * scale, 10 * scale);
            draw_list->AddRectFilled(
                ImVec2(loading_pos.x - 10 * scale, loading_pos.y - 5 * scale),
                ImVec2(loading_pos.x + 210 * scale, loading_pos.y + 25 * scale),
                IM_COL32(230, 126, 34, 220), 8.0f * scale
            );
            draw_list->AddText(loading_pos, IM_COL32(0, 0, 0, 255), "Downloading map tiles...");
        }

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
        GLuint MissionPlanner_Icon = theme ? images.at("job_white") : images.at("job");

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
        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 160 * scale),"Mission Planner",scale, MissionPlanner_Icon, theme, 0, 2)) {
            panel = 1;

        }
        GLuint Placeholder_Icon = images.at("drone");  // no light-theme variant yet -- same icon either way

        if (panel == 2) {
            ImVec2 highlight_center = ImVec2(30 * scale, 220 * scale);
            float highlight_size = (26 + 2) * scale;
            draw_list->AddRectFilled(
                ImVec2(highlight_center.x - highlight_size, highlight_center.y - highlight_size),
                ImVec2(highlight_center.x + highlight_size, highlight_center.y + highlight_size),
                IM_COL32(255, 130, 30, 255),
                12.0f
            );
        }
        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 220 * scale),"Placeholder",scale, Placeholder_Icon, theme, 0, 2)) {
            sat_map = !sat_map;

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

        if (widgets.ModeToggle(draw_list, ImVec2(160 * scale, 30 * scale), scale, theme, is_manual)) {
            
        }
        
        EndFixedPanel(); 

        switch (panel)
        {

        case 0:
        {
        //--------------------------MAP--------------------------------------------------------------
        if(sat_map){
            BeginFixedPanel("MapPanel", ImVec2(70 * scale, 70 * scale), ImVec2(1500 * scale, 800 * scale),
                scale, theme, 0, ImVec2(0, 0));
            location.MapWidget(Info.gps_status.latitude, Info.gps_status.longitude, 1500 * scale, 900 * scale, scale, map_zoom, placeholderTile, theme);


            if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
                std::cout << "ESTOP Button Clicked!" << std::endl;
            }

            ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
            widgets.AltitudeTape(1, (-1 * Info.xyz_pos[2]), 0.5f, theme, scale); 

            widgets.GyroScopeIndicator(draw_list,
                                    ImVec2(1320 * scale, 810 * scale),
                                    Info.orientation, 
                                    theme, scale);

            widgets.Compas(draw_list,
                                    ImVec2(1440 * scale, 810 * scale),
                                    Info.orientation, 
                                    theme, scale);                   

            EndFixedPanel();
        } else{
            BeginFixedPanel("NoSatMapPanel", ImVec2(70 * scale, 70 * scale), ImVec2(1500 * scale, 800 * scale),
                scale, theme, 0, ImVec2(0, 0));
            location.NoSatMap(Info.gps_status.latitude, Info.gps_status.longitude, 1500 * scale, 900 * scale, scale, map_zoom, placeholderTile, theme);
             if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
                std::cout << "ESTOP Button Clicked!" << std::endl;
            }

            ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
            widgets.AltitudeTape(1, (-1 * Info.xyz_pos[2]), 0.5f, theme, scale); 

            widgets.GyroScopeIndicator(draw_list,
                                    ImVec2(1320 * scale, 810 * scale),
                                    Info.orientation, 
                                    theme, scale);

            widgets.Compas(draw_list,
                                    ImVec2(1440 * scale, 810 * scale),
                                    Info.orientation, 
                                    theme, scale); 
            EndFixedPanel();
        }
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
    
        
        //ImGui::InputDouble("Lat", &testLat, 0.000001, 0.01, "%.7f");
        //ImGui::InputDouble("Lon", &testLon, 0.000001, 0.01, "%.7f");
        //ImGui::SliderFloat("Altitude", &Info.xyz_pos[2], -20.0f, 20.0f);
        //float yaw_f = (float)Info.orientation.yaw;
        //if (ImGui::SliderFloat("Yaw", &yaw_f, -180.0f, 180.0f)) {
        //    Info.orientation.yaw = yaw_f;
        //}
//
        //float roll_f = (float)Info.orientation.roll;
        //if (ImGui::SliderFloat("Roll", &roll_f, -180.0f, 180.0f)) {
        //    Info.orientation.roll = roll_f;
        //}
//
        //float pitch_f = (float)Info.orientation.pitch;
        //if (ImGui::SliderFloat("Pitch", &pitch_f, -180.0f, 180.0f)) {
        //    Info.orientation.pitch = pitch_f;
        //}
        ImGui::SliderFloat("BatteryVoltage", &Info.battery_values_C[0], 0, 1.0f);



        ImGui::EndChild();
        

       

        //--------------------------------------Information panels---------------------------------------
        info_panels.Battery_Info(scale, theme, Info.battery_values_C);
        
        info_panels.Position_Info(scale, theme, Info.xyz_pos);
        info_panels.Probe_Info(scale, theme);
        
        
        info_panels.ResetPanelTracking();



        break;
        }
        case 1:
        {   

            planner_panel.Draw(scale, theme);

            // Static view for now -- no click-to-set-waypoint or plan
            // markers yet, that's a separate follow-up.
            // The left panel's 70*scale gap from the window edge exists to
            // clear the sidebar -- there's no equivalent on the right, so
            // mirroring that same 70px here just left dead space. Instead,
            // reuse the existing 10px rhythm already established between
            // VehicleAndPalettePanel and MissionPlannerPanel (bottom of the
            // former at 70+140=210, top of the latter at 220), applied both
            // between the planner panel and the map, and between the map
            // and the window's right edge.
            const float map_gap = 10.0f * scale;
            const float planner_right_edge = (70.0f + 450.0f) * scale;  // matches DrawTaskList's panel rect
            const float map_x = planner_right_edge + map_gap;
            const float map_right_margin = map_gap;
            const float map_w = std::max(200.0f * scale, static_cast<float>(x_sc) - map_x - map_right_margin);
            if(sat_map){
                
                BeginFixedPanel("PlannerMapPanel", ImVec2(map_x, 70 * scale), ImVec2(map_w, 800 * scale),
                        scale, theme, 0, ImVec2(0, 0));
                location.MapWidget(Info.gps_status.latitude, Info.gps_status.longitude, map_w, 800 * scale, scale, map_zoom, placeholderTile, theme);
                EndFixedPanel();

                BeginOverlayPanel(draw_list, "PlannerMapUtilsPanel", ImVec2(map_x + map_w - 70.0f * scale, 72 * scale), ImVec2(49 * scale, 150 * scale), scale, theme);
                GLuint PlannerPlus_Icon = theme ? images.at("plus_white") : images.at("plus");
                if (widgets.CustomButton(draw_list, ImVec2(map_x + map_w - 46.0f * scale, 97 * scale), "Plus", scale, PlannerPlus_Icon, theme, -5, -3)) {
                    if (map_zoom < 20) {
                        map_zoom += 1;
                    }
                }
                GLuint PlannerMinus_Icon = theme ? images.at("minus_white") : images.at("minus");
                if (widgets.CustomButton(draw_list, ImVec2(map_x + map_w - 46.0f * scale, 141 * scale), "Minus", scale, PlannerMinus_Icon, theme, -5, -3)) {
                    if (map_zoom > 13) {
                        map_zoom -= 1;
                    }
                }
                EndOverlayPanel();

            }
            else{
                BeginFixedPanel("PlannerSatMapPanel", ImVec2(map_x, 70 * scale), ImVec2(map_w, 800 * scale),
                        scale, theme, 0, ImVec2(0, 0));
               location.NoSatMap(Info.gps_status.latitude, Info.gps_status.longitude, map_w, 800 * scale, scale, map_zoom, placeholderTile, theme); 
               EndFixedPanel();

            }
           
            // Altitude profile, same x-span as the map, directly beneath
            // it. Height fills to the actual window edge (mirroring the
            // map's own width fix) rather than a fixed 150*scale, which
            // only used the reference height exactly and otherwise left a
            // gap before the true bottom edge on any other resolution.
            const float chart_y = 880.0f * scale;
            const float chart_h = std::max(80.0f * scale, static_cast<float>(y_sc) - chart_y - map_gap);
            DrawHeightChart(planner.plan(), ImVec2(map_x, chart_y), ImVec2(map_w, chart_h), scale, theme);
        }
        break;
        case 2:
        {
            // Reserved -- empty for now.
        }
        break;
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
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    rclcpp::shutdown();  
    return 0;
}