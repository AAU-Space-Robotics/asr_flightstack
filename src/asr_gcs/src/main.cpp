#include <iostream>
#include "interfaceutils.h"
#include "transformations.h"
#include "state_manager.h"
#include "app_window.h"
#include "info_panels.h"
#include "map.h"
#include "planner/map_overlay.h"
#include "widgets.h"
#include "manual.h"
#include "camera.h"
#include "planner/planner.h"
#include "planner/planner_panel.h"
#include "planner/mission_control_bar.h"
#include "planner/height_chart.h"
#include "prearm_checklist.h"
#include "analyze/analyze_panel.h"
#include <implot.h>


#include <algorithm>
#include <opencv2/core/utils/logger.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <thread>
#include <cstdlib>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <rclcpp/rclcpp.hpp>
#include <rcutils/logging.h>
#include <asr_comms/msg/telemetry_battery.hpp>
#include <asr_comms/msg/telemetry_gps.hpp>
#include "asr_comms/msg/telemetry_position.hpp"
#include "asr_comms/msg/telemetry_attitude.hpp"
#include "asr_comms/msg/telemetry_battery.hpp"
#include "asr_comms/msg/telemetry_gps.hpp"
#include "asr_comms/msg/telemetry_origin_gps.hpp"
#include "asr_comms/msg/telemetry_status.hpp"
#include "asr_comms/msg/base_station_stats.hpp"
#include "asr_comms/msg/telemetry_status.hpp"
#include "asr_comms/msg/gcs_heartbeat.hpp"
#include "asr_comms/msg/uav_command.hpp"
#include "asr_comms/msg/command_ack.hpp"
#include "asr_comms/msg/servo_command.hpp"
#include "asr_comms/msg/probe_locations.hpp"
#include "asr_comms/msg/gcs_heartbeat.hpp"
#include "asr_comms/msg/manual_control_input.hpp"
#include "asr_comms/msg/link_stats.hpp"
#include "asr_comms/msg/command_ack.hpp"

#include "sensor_msgs/msg/compressed_image.hpp"
#include "asr_comms/msg/camera_stream_request.hpp"

#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/distance_sensor.hpp>

         

WindowInitializer winInit;
Widgets widgets;
Location location;
Color colors;
TestFunc test_functions;
InfoPanels info_panels;
Logs logs;
ManualController manual_controller;
cv::Mat frame;

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
        //RCLCPP_INFO(get_logger(), "GCS Started");
        rclcpp::QoS qos(10);
        qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
        qos.durability(rclcpp::DurabilityPolicy::TransientLocal);
        rclcpp::QoS qos_volatile(10);
        qos_volatile.reliability(rclcpp::ReliabilityPolicy::BestEffort);
        qos_volatile.durability(rclcpp::DurabilityPolicy::Volatile);

        rclcpp::QoS camera_qos(10);
        camera_qos.reliability(rclcpp::ReliabilityPolicy::BestEffort);
        camera_qos.durability(rclcpp::DurabilityPolicy::Volatile);

        bool use_sim_time = false;
        clock_ = std::make_shared<rclcpp::Clock>(use_sim_time ? RCL_ROS_TIME : RCL_SYSTEM_TIME);



        std::cout << "\n"
          << "=============================\n"
          << "    * GCS STARTING... *\n"
          << "=============================\n"
          << std::endl;
        //------------------------Subscribtions---------------
        attitude_sub_ = create_subscription<asr_comms::msg::TelemetryAttitude>(
            "telemetry/attitude", qos_volatile,
            [this](const asr_comms::msg::TelemetryAttitude::SharedPtr msg)
            { attitudeCallback(msg); });
        local_position_sub_ = create_subscription<asr_comms::msg::TelemetryPosition>(
            "telemetry/position", qos_volatile,
            [this](const asr_comms::msg::TelemetryPosition::SharedPtr msg) { localPositionCallback(msg); });
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
        gps_sub_ = create_subscription<asr_comms::msg::TelemetryGPS>(
            "telemetry/gps", 10,
            [this](const asr_comms::msg::TelemetryGPS::SharedPtr msg)
            { gpsCallback(msg); });
        origin_gps_sub_ = create_subscription<asr_comms::msg::TelemetryOriginGPS>(
            "telemetry/origin_gps", 10,
            [this](const asr_comms::msg::TelemetryOriginGPS::SharedPtr msg) { originGpsCallback(msg); });
        base_station_sub_ = create_subscription<asr_comms::msg::BaseStationStats>(
            "basestation_stats", 10,
            [this](const asr_comms::msg::BaseStationStats::SharedPtr msg) { basestationCallback(msg); });
        status_sub_ = create_subscription<asr_comms::msg::TelemetryStatus>(
            "telemetry/status", 10,
            [this](const asr_comms::msg::TelemetryStatus::SharedPtr msg) { statusCallback(msg); });
        probe_sub_ = create_subscription<asr_comms::msg::ProbeLocations>(
            "probe/probe_locations", 10,
            [this](const asr_comms::msg::ProbeLocations::SharedPtr msg) { probeCallback(msg); });
        link_stats_sub_ = create_subscription<asr_comms::msg::LinkStats>(
            "link_stats", 10,
            [this](const asr_comms::msg::LinkStats::SharedPtr msg) { linkStatusCallback(msg); });
        command_ack_sub_ = create_subscription<asr_comms::msg::CommandAck>(
            "command_ack", 10,
            [this](const asr_comms::msg::CommandAck::SharedPtr msg) { commandAckCallback(msg); });
        camera_sub_ = create_subscription<sensor_msgs::msg::CompressedImage>(
            "camera/image/compressed", 10,
            [this](const sensor_msgs::msg::CompressedImage::SharedPtr msg) { cameraCallback(msg); });


        heartbeat_pub_ = create_publisher<GcsHeartbeat>("in/gcs_heartbeat", qos);
        heartbeat_timer_ = create_wall_timer(
            std::chrono::milliseconds(500),
            [this]() { heartbeat(); }
        );
        manual_control_pub_ = create_publisher<asr_comms::msg::ManualControlInput>("in/manual_input", qos);

        uav_command_pub_ = create_publisher<asr_comms::msg::UAVCommand>("in/uav_command", 10);

        camera_stream_pub_ = create_publisher<asr_comms::msg::CameraStreamRequest>("in/camera_stream", 1);

        require_checklist_ = declare_parameter<bool>("require_checklist", true);
        min_battery_percent_ = declare_parameter<double>("min_battery_percent", 40.0);
        min_satellites_ = declare_parameter<int>("min_satellites", 6);
        
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
    void send_command(string command_type, vector<double> target_pose = {}, double yaw = 0.0){
        asr_comms::msg::UAVCommand msg;
        msg.command_type = command_type;
        msg.target_pose = target_pose;
        msg.yaw = yaw;

        uav_command_pub_->publish(msg);
    }
    void send_manual_control(float roll, float pitch, float yaw_velocity, float thrust, bool manual_engage)
    {
        auto msg = asr_comms::msg::ManualControlInput();
        msg.roll = roll;
        msg.pitch = pitch;
        msg.yaw_velocity = yaw_velocity;
        msg.thrust = thrust;
        msg.manual_engage = manual_engage ? 1 : 0;

        RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 500,
            "Sending manual control: roll=%.3f pitch=%.3f yaw=%.3f thrust=%.3f engage=%d",
            roll, pitch, yaw_velocity, thrust, msg.manual_engage);

        manual_control_pub_->publish(msg);
    }
    FlightMode getFlightMode() {
        std::lock_guard<std::mutex> lock(flight_mode_mutex_);
        return current_flight_mode_;
    }
    LinkStatus getLinkStatus() {
        std::lock_guard<std::mutex> lock(link_status_mutex_);
        return link_status_;
    }
    bool requireChecklist() const { return require_checklist_; }
    double minBatteryPercent() const { return min_battery_percent_; }
    int minSatellites() const { return min_satellites_; }
    bool getEstop() const {
        std::lock_guard<std::mutex> lock(estop_mutex_);
        return estop_engaged_;
    }
    void addLogLine(LogLevel level,  const std::string& message)
    {
        std::lock_guard<std::mutex> lock(log_mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);

        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%H:%M:%S");

        LogEntry entry;
        entry.timestamp = oss.str();
        entry.level = level;
        entry.message = message;

        log_lines_.push_back(entry);
        if (log_lines_.size() > MAX_LOG_LINES) {
            log_lines_.erase(log_lines_.begin());   
        }
        
    }
    std::vector<LogEntry> getLogLines()
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        return log_lines_;
    }
    void clearLog()
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_lines_.clear();
    }
    cv::Mat getCameraFrame()
    {
        std::lock_guard<std::mutex> lock(camera_mutex_);
        return latest_camera_frame_.clone();
    }
    void setCameraStreaming(bool enabled)
    {
        auto msg = asr_comms::msg::CameraStreamRequest();
        msg.enabled = enabled;
        camera_stream_pub_->publish(msg);
        addLogLine(LogLevel::INFO, enabled ? "Camera stream start requested" : "Camera stream stop requested");
    }
private:
    StateManager state_manager_;
    Transformations transformations_;
    std::thread execute_thread_;
    rclcpp::Subscription<asr_comms::msg::TelemetryAttitude>::SharedPtr attitude_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryPosition>::SharedPtr local_position_sub_;
    std::shared_ptr<rclcpp::Clock> clock_;
    rclcpp::Subscription<DistanceSensor>::SharedPtr distance_sensor_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_compute_;
    rclcpp::Subscription<asr_comms::msg::TelemetryBattery>::SharedPtr battery_sub_main_;
    rclcpp::Subscription<asr_comms::msg::TelemetryGPS>::SharedPtr gps_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryOriginGPS>::SharedPtr origin_gps_sub_;
    rclcpp::Subscription<asr_comms::msg::BaseStationStats>::SharedPtr base_station_sub_;
    rclcpp::Subscription<asr_comms::msg::TelemetryStatus>::SharedPtr status_sub_;
    rclcpp::Subscription<asr_comms::msg::ProbeLocations>::SharedPtr probe_sub_;
    rclcpp::Subscription<asr_comms::msg::LinkStats>::SharedPtr link_stats_sub_;
    rclcpp::Subscription<asr_comms::msg::CommandAck>::SharedPtr command_ack_sub_;
    rclcpp::Publisher<GcsHeartbeat>::SharedPtr heartbeat_pub_;
    rclcpp::Publisher<asr_comms::msg::ManualControlInput>::SharedPtr manual_control_pub_;
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    rclcpp::Publisher<asr_comms::msg::UAVCommand>::SharedPtr uav_command_pub_;
    
    cv::Mat latest_camera_frame_;
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr camera_sub_;
    rclcpp::Publisher<asr_comms::msg::CameraStreamRequest>::SharedPtr camera_stream_pub_;
    bool new_camera_frame_available_ = false;
    mutable std::mutex camera_mutex_;
      

    FlightMode current_flight_mode_ = FlightMode::STANDBY;
    mutable std::mutex flight_mode_mutex_;

    LinkStatus link_status_;
    mutable std::mutex link_status_mutex_;

    bool require_checklist_ = true;
    double min_battery_percent_ = 40.0;
    int min_satellites_ = 6;

    bool estop_engaged_ = false;
    mutable std::mutex estop_mutex_;

    std::vector<LogEntry> log_lines_;
    mutable std::mutex log_mutex_;
    static constexpr size_t MAX_LOG_LINES = 200;

    
    


    void attitudeCallback(const asr_comms::msg::TelemetryAttitude::SharedPtr msg)
    {
        if (!std::isfinite(msg->orientation[0]) || !std::isfinite(msg->orientation[1]) ||
            !std::isfinite(msg->orientation[2])) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
                "Rejecting non-finite attitude");
            return;
        }

        Eigen::Quaterniond quat = transformations_.eulerToQuaternion(
            msg->orientation[0], msg->orientation[1], msg->orientation[2]).normalized();

        StampedQuaternion attitude(get_time(), quat);
        state_manager_.setAttitude(attitude);
    }
   void localPositionCallback(const asr_comms::msg::TelemetryPosition::SharedPtr msg)
    {
        if (!std::isfinite(msg->position[0]) || !std::isfinite(msg->position[1]) || !std::isfinite(msg->position[2])) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
                "Rejecting invalid local position");
            return;
        }

        // Note that local position refers to coordinates being expressed in cartesian coordinates from some origin point.
        Stamped3DVector origin = state_manager_.getOrigin();
        Stamped3DVector local_position(get_time(), msg->position[0] - origin.x(), msg->position[1] - origin.y(), msg->position[2] - origin.z());
        state_manager_.setGlobalPosition(local_position);
        Stamped4DVector target_position(get_time(), msg->target_position[0], msg->target_position[1], msg->target_position[2], 0.0);
        state_manager_.setTargetPositionProfile(target_position);
        if (std::isfinite(msg->velocity[0]) && std::isfinite(msg->velocity[1]) && std::isfinite(msg->velocity[2])) {
            state_manager_.setGlobalVelocity(Stamped3DVector(get_time(), msg->velocity[0], msg->velocity[1], msg->velocity[2]));
        } else {
            state_manager_.setGlobalVelocity(Stamped3DVector(get_time(), 0.0, 0.0, 0.0));
        }

        if (std::isfinite(msg->distance_sensor)) {
            state_manager_.setGroundDistanceState(Stamped3DVector(get_time(), msg->distance_sensor, 0.0, 0.0));
        }
    }
    void distanceSensorCallback(const DistanceSensor::SharedPtr msg)
    {
       
        if (std::isfinite(msg->current_distance)) {
            state_manager_.setGroundDistanceState(Stamped3DVector(get_time(), msg->current_distance, 0.0, 0.0));
        }
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
    void gpsCallback(const asr_comms::msg::TelemetryGPS::SharedPtr msg)
    {
        GPSState gps_state;
        gps_state.latitude = msg->latitude;
        gps_state.longitude = msg->longitude;
        gps_state.satellites_used = msg->satellites_used;
        state_manager_.setGPSState(gps_state);

    }
    void originGpsCallback(const asr_comms::msg::TelemetryOriginGPS::SharedPtr msg)
    {
        state_manager_.setOriginGPS(Stamped3DVector(get_time(), msg->latitude, msg->longitude, msg->altitude));
    }
    void basestationCallback(const asr_comms::msg::BaseStationStats::SharedPtr msg)
    {
        BaseStateInfo RTK_info;
        RTK_info.bs_connected = msg->basestation_connected;
        RTK_info.bs_status = static_cast<RTK_STATUS>(msg->rtk_status);
        RTK_info.rtk_accuracy = msg-> rtk_accuracy;
        RTK_info.rtk_accuracy_target = msg->rtk_accuracy_target;
        RTK_info.rtk_survey_duration = msg->rtk_survey_duration;

        state_manager_.setRTKstatus(RTK_info);

    }

    void linkStatusCallback(const asr_comms::msg::LinkStats::SharedPtr msg)
    {
        LinkStatus status;
        status.mavlink_connected = msg->mavlink_connected;
        status.wifi_connected    = msg->wifi_connected;
        status.camera_streaming  = msg->camera_streaming;
        status.camera_rx_kbps    = msg->camera_rx_kbps;
        status.signal_quality    = msg->signal_quality;

        std::lock_guard<std::mutex> lock(link_status_mutex_);
        link_status_ = status;
    }


    void statusCallback(const asr_comms::msg::TelemetryStatus::SharedPtr msg)
    {
        state_manager_.setArminState(msg->arming_state);
        {
            std::lock_guard<std::mutex> lock(flight_mode_mutex_);
            current_flight_mode_ = static_cast<FlightMode>(msg->flight_mode);
        }
        {
            std::lock_guard<std::mutex> lock(estop_mutex_);
            estop_engaged_ = (msg->estop != 0);
        }

        Stamped4DVector actuator_speed(get_time(),
            msg->actuator_speeds[0],
            msg->actuator_speeds[1],
            msg->actuator_speeds[2],
            msg->actuator_speeds[3]);

        state_manager_.setActuatorSpeeds(actuator_speed);
    }
    void probeCallback(const asr_comms::msg::ProbeLocations::SharedPtr msg )
    {
        ProbeData data;
        data.probes.reserve(msg->num_probes);   

        for (uint32_t i = 0; i < msg->num_probes; i++) {
            Probe p;
            p.x = msg->positions[i * 3 + 0];
            p.y = msg->positions[i * 3 + 1];
            p.z = msg->positions[i * 3 + 2];

            p.sigma_x = msg->uncertainty[i * 3 + 0];
            p.sigma_y = msg->uncertainty[i * 3 + 1];
            p.sigma_z = msg->uncertainty[i * 3 + 2];

            data.probes.push_back(p);
            
        }

        state_manager_.setInterfaceProbeLocations(data);
    }
    void heartbeat()
    {
        auto msg = GcsHeartbeat();
        msg.timestamp = static_cast<double>(get_time().nanoseconds()) / 1e9;
        msg.gcs_nominal = 1;

        heartbeat_pub_->publish(msg);
    }

    std::string MavResultToText(uint8_t result)
    {
        switch (result) {
            case 0: return "ACCEPTED";
            case 1: return "TEMP_REJECTED";
            case 2: return "DENIED";
            case 3: return "UNSUPPORTED";
            case 4: return "FAILED";
            case 6: return "CANCELLED";
            default: return "UNKNOWN";
        }
    }
    void commandAckCallback(const asr_comms::msg::CommandAck::SharedPtr msg)
    {
        LogLevel level = (msg->result == 0) ? LogLevel::INFO : LogLevel::ERROR;
        std::string log_message = msg->command_type + " -> " + MavResultToText(msg->result);
        addLogLine(level, log_message);
    }
    void cameraCallback(const sensor_msgs::msg::CompressedImage::SharedPtr msg)
    {
        cv::Mat encoded(1, msg->data.size(), CV_8UC1, const_cast<uint8_t*>(msg->data.data()));
        cv::Mat decoded = cv::imdecode(encoded, cv::IMREAD_COLOR);
        if (decoded.empty()) return;
    
        std::lock_guard<std::mutex> lock(camera_mutex_);
        latest_camera_frame_ = decoded;
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
    manual_controller.InitJoystick();
    Planner planner(ground_control.get());
    PlannerPanel planner_panel(planner);
    MissionControlBar mission_control_bar;
    PreArmChecklist prearm_checklist;
    analyze::AnalyzePanel analyze_panel;
    bool arming_state = false;
    const bool require_checklist = ground_control->requireChecklist();
    const double min_battery_percent = ground_control->minBatteryPercent();
    const int min_satellites = ground_control->minSatellites();
    auto guard_with_checklist = [&](std::function<void()> action) {
        if (!arming_state && require_checklist) {
            prearm_checklist.Open(std::move(action));
        } else {
            action();
        }
    };
    auto request_arm = [&]() {
        guard_with_checklist([ground_control]() {
            ground_control->send_command("arm");
            ground_control->addLogLine(LogLevel::INFO, "Arm command sent");
        });
    };
    DroneInformation Info;
    Transformations transformations_;

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    bool armButton = false;
    bool theme = 1;
    int map_zoom = 20;
    double Latitude = 57.063, Longitude = 10.032;
      // shared between FlightMode and Mission Planner map views
    int panel = 0;
    //bool is_manual = (Info.flight_mode == FlightMode::MANUAL_AIDED); for later:
    bool is_manual = false;
    bool sat_map = 1;
    int map_size_x = 800;
    string goto_text;
    FlightMode pending_mode = FlightMode::STANDBY;
    bool mode_switch_pending = false;
    rclcpp::Time mode_switch_start;
    static rclcpp::Time last_estop_log_time = ground_control->get_time();

    bool prev_controller_arm_button = false;
    bool prev_controller_land_button = false;

    std::atomic<bool> tiles_downloading{false};
    bool tiles_download_triggered = false;

    GLuint camera_texture_ = 0; 

     static bool camera_stream_requested = false;
    

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
    glfwSwapInterval(0); // Enable vsync
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImPlot::CreateContext();  
    winInit.loadFonts(); // Load fonts once

    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = false;
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
        {"drone",       "droneImage.png"},  // placeholder icon
        {"switch_white",      "switch_white.png"},
        {"switch",      "switch.png"},
        {"send",        "send.png"},
        {"goto",        "goto.png"},
        {"goto_white",   "goto_white.png"},
        {"analyze",      "problem-solving.png"},
        {"camera",          "camera.png"},
        {"camera_white",          "camera_white.png"}
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
        glfwSetErrorCallback([](int error, const char* description) {
            if (error == GLFW_INVALID_VALUE && std::string(description).find("gamepad mapping") != std::string::npos) {
                return;
            }
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
        });
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
        const Stamped3DVector& position = ground_control->getStateManager().getGlobalPosition();
        const Stamped4DVector& target = ground_control->getStateManager().getTargetPositionProfile();
        const Stamped3DVector& velocity = ground_control->getStateManager().getGlobalVelocity();
        const Stamped4DVector& speeds = ground_control->getStateManager().getActuatorSpeeds();
        const Stamped3DVector& ground_distance = ground_control->getStateManager().getGroundDistanceState();
        const Stamped3DVector origin_gps = ground_control->getStateManager().getOriginGPS();
        LinkStatus LS = ground_control->getLinkStatus();
        JoystickState js = manual_controller.PollJoystick();
        
        Info.orientation = quaternionToEulerForDisplay(attitude.quaternion()); 
        Info.ground_distance = static_cast<float>(ground_distance.x());
        Info.xyz_pos[0] = static_cast<float>(position.x());
        Info.xyz_pos[1] = static_cast<float>(position.y());
        Info.xyz_pos[2] = static_cast<float>(position.z());
       
        Info.tgt_pos[0] = static_cast<float>(target.x());
        Info.tgt_pos[1] = static_cast<float>(target.y());
        Info.tgt_pos[2] = static_cast<float>(target.z());

        Info.velocity[0] = static_cast<float>(velocity.x());
        Info.velocity[1] = static_cast<float>(velocity.y());
        Info.velocity[2] = static_cast<float>(velocity.z());
        Info.gps_status = ground_control->getStateManager().getGPSState();
        Info.battery_values_M = ground_control->getStateManager().getBatteryState(BATTERY_MAIN);
        Info.battery_values_C = ground_control->getStateManager().getBatteryState(BATTERY_COMPUTE);
        Info.motor_speed[0] = speeds.x();
        Info.motor_speed[1] = speeds.y();
        Info.motor_speed[2] = speeds.z();
        Info.motor_speed[3] = speeds.w();
        Info.RTK_INFO = ground_control->getStateManager().getRTKstatus();
        Info.flight_mode = ground_control->getFlightMode();
        Info.probes = ground_control->getStateManager().getInterfaceProbeLocations();
        Info.estop = ground_control->getEstop();

        PreArmSnapshot prearm_snapshot;
        prearm_snapshot.rtk_status = Info.RTK_INFO.bs_status;
        prearm_snapshot.battery_percent = Info.battery_values_M.charge_remaining * 100.0;  // charge_remaining is a 0-1 fraction (PX4 convention)
        prearm_snapshot.min_battery_percent = static_cast<float>(min_battery_percent);
        prearm_snapshot.estop_engaged = Info.estop;
        prearm_snapshot.satellites_used = Info.gps_status.satellites_used;
        prearm_snapshot.min_satellites = min_satellites;
        const std::vector<std::string> prearm_hard_issues = PreArmHardBlockIssues(prearm_snapshot);
        bool actually_streaming = ground_control->getLinkStatus().camera_streaming;
        
        
        if (!mode_switch_pending) {   
            is_manual = (Info.flight_mode == FlightMode::MANUAL_AIDED || Info.flight_mode == FlightMode::MANUAL);
        }
        arming_state = ground_control->getStateManager().getArminState();

        
       
        

        

        bool gps_looks_valid = (Info.gps_status.satellites_used >= 4) &&
                       (Info.gps_status.latitude != 0.0) &&
                       (Info.gps_status.longitude != 0.0);
        
        if (gps_looks_valid && !tiles_download_triggered) {
            tiles_download_triggered = true;
            double lat = Info.gps_status.latitude;
            double lon = Info.gps_status.longitude;
            ground_control->addLogLine(LogLevel::INFO,"Valid GPS coordinates");

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
            ImVec2 loading_pos = ImVec2(x_sc / 2.0f - 100 * scale, 90 * scale);
            draw_list->AddRectFilled(
                ImVec2(loading_pos.x - 10 * scale, loading_pos.y - 5 * scale),
                ImVec2(loading_pos.x + 210 * scale, loading_pos.y + 25 * scale),
                IM_COL32(230, 126, 34, 220), 8.0f * scale
            );
            draw_list->AddText(loading_pos, IM_COL32(0, 0, 0, 255), "Downloading map tiles...");
        }

        cv::Mat frame = ground_control->getCameraFrame();
        if (!frame.empty()) {
            if (camera_texture_ != 0) glDeleteTextures(1, &camera_texture_);
            camera_texture_ = UploadCameraFrameToGL(frame);
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
          // cout << "howdi" <<endl;
          // ground_control->addLogLine(LogLevel::ERROR,"Howdi");
           panel = 2;

        }

        GLuint Analyze_Icon = images.at("analyze");  // no light-theme variant yet -- same icon either way

        if (panel == 3) {
            ImVec2 highlight_center = ImVec2(30 * scale, 280 * scale);
            float highlight_size = (26 + 2) * scale;
            draw_list->AddRectFilled(
                ImVec2(highlight_center.x - highlight_size, highlight_center.y - highlight_size),
                ImVec2(highlight_center.x + highlight_size, highlight_center.y + highlight_size),
                IM_COL32(255, 130, 30, 255),
                12.0f
            );
        }
        if (widgets.CustomButton(draw_list, ImVec2(30 * scale, 280 * scale),"Analyze",scale, Analyze_Icon, theme, 0, 2)) {
            panel = 3;
        }


        EndFixedPanel();

        
        // ----------------------------------------Top Panel-----------------------------------------
        BeginFixedPanel("TopPanel", ImVec2(-20 * scale, -20 * scale), ImVec2(x_sc + 1000 * scale, 80 * scale),
                scale, theme, 0, ImVec2(0, 0));
        
        draw_list->AddImageRounded(
            (ImTextureID)(intptr_t)images.at("aau_logo"),
            ImVec2(8 * scale, 8 * scale),  
            ImVec2(55 * scale, 55 * scale),  // inset max
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 200),
            12.0f
        );
        for (int i = 0; i <= 208; i+=208){
            draw_list->AddLine(
                ImVec2((59 + i) * scale, 5 * scale),
                ImVec2((59 + i) * scale, 50 * scale),   
                colors.panelBorder(theme),
                2.0f
            );
        }
        if (widgets.ArmButton(draw_list,ImVec2(342 * scale, 30 * scale), scale, theme, arming_state )){
            if(!arming_state){
                request_arm();
            } else {
                ground_control->send_command("disarm");
                ground_control->addLogLine(LogLevel::INFO,"Disarm command sent");
            }

        }

        if (require_checklist && !arming_state) {
            DrawIssuesBadge(draw_list, ImVec2(400 * scale, 12 * scale), scale,
                             static_cast<int>(prearm_hard_issues.size()));
        }

        if (widgets.ModeToggle(draw_list, ImVec2(164 * scale, 30 * scale), scale, theme, is_manual)) {
            if (is_manual) {
                
                if (js.connected) {
                    ground_control->send_command("manual_aided");
                    ground_control->addLogLine(LogLevel::INFO,"Manual-aided mode initialized");
                    pending_mode = FlightMode::MANUAL_AIDED;
                    mode_switch_pending = true;
                    mode_switch_start = ground_control->get_time();
                } else {
                    ground_control->addLogLine(LogLevel::ERROR,"Joystick not connected or not initialized");
                    is_manual = false;   // don't leave the toggle showing "Manual" if we didn't actually send the command
                }
            }
        }
        
        EndFixedPanel();

        if (js.connected) {
            ManualControlValues ctrl = manual_controller.ComputeManualControl(js);

            ground_control->send_manual_control(ctrl.roll, ctrl.pitch, ctrl.yaw_velocity, ctrl.thrust, ctrl.manual_engage);

            // Arm/land on rising edge only. It should not resend every frame the button is held.
            if (ctrl.arm_pressed && !prev_controller_arm_button) {
                request_arm();
            }
            prev_controller_arm_button = ctrl.arm_pressed;

            if (ctrl.land_pressed && !prev_controller_land_button) {
                ground_control->send_command("land");
                ground_control->addLogLine(LogLevel::INFO, "Land command sent");
            }
            prev_controller_land_button = ctrl.land_pressed;

            // Estop: keep re-asserting every frame while held, never rate-limited.
            if (ctrl.estop_pressed) {
                ground_control->send_command("estop");
                auto now = ground_control->get_time();
                if ((now - last_estop_log_time).seconds() > 1.0) {   
                    ground_control->addLogLine(LogLevel::WARN, "Estop command sent");
                    last_estop_log_time = now;
                }
            }
        }
        
        
        widgets.SurveyPanel(draw_list, ImVec2(1450, 22), scale, theme, Info.RTK_INFO.bs_status, Info.RTK_INFO.bs_connected, js, LS);
        switch (panel)
        {

        case 0:
        {
        const float mission_bar_top_y = mission_control_bar.PanelTopY(scale, 70 * scale, 800 * scale);

        //--------------------------MAP--------------------------------------------------------------
        if(sat_map){
            BeginFixedPanel("MapPanel", ImVec2(70 * scale, 70 * scale), ImVec2(1500 * scale, map_size_x * scale),
                scale, theme, 0, ImVec2(0, 0));
            ImVec2 front_map_pos = location.MapWidget(origin_gps.x(), origin_gps.y(), 1500 * scale, 900 * scale, scale, map_zoom, placeholderTile, theme);
            // No task list here to sync selection with, so no highlight and the click result is unused.
            const float overlay_h = std::max(80.0f * scale, mission_bar_top_y - front_map_pos.y);
            // Foreground-drawn, so it renders above modals too -- skip while Load/Save is up, or it bleeds through.
            if (!mission_control_bar.IsDialogOpen()) {
                DrawPlanRouteOverlay(location, planner.plan(), origin_gps.x(), origin_gps.y(),
                                     origin_gps.x(), origin_gps.y(), map_zoom,
                                     front_map_pos, 1500 * scale, 900 * scale, overlay_h, scale, -1, -1);
                DrawUavPositionMarker(location, Info.gps_status.latitude, Info.gps_status.longitude,
                                      origin_gps.x(), origin_gps.y(), map_zoom,
                                      front_map_pos, 1500 * scale, 900 * scale, overlay_h, scale);
            }


            if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
                ground_control->send_command("estop"); 
                auto now = ground_control->get_time();
                if ((now - last_estop_log_time).seconds() > 1.0) {   
                    ground_control->addLogLine(LogLevel::WARN, "Estop command sent");
                    last_estop_log_time = now;
                }
            }

            ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
            widgets.AltitudeTape(1, Info.ground_distance, 0.5f, theme, scale);

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
            BeginFixedPanel("NoSatMapPanel", ImVec2(70 * scale, 70 * scale), ImVec2(1500 * scale, map_size_x * scale),
                scale, theme, 0, ImVec2(0, 0));
            location.NoSatMap(Info, 1500 * scale, map_size_x * scale, scale, map_zoom, theme);
             if (widgets.DrawCircleGradientButton(draw_list, winInit.getFont(24), 1.0f, ImVec2(130 * scale, 125 * scale), 50.0f * scale, "ESTOP", 40.0f * scale)) {
                ground_control->send_command("estop"); 
                auto now = ground_control->get_time();
                if ((now - last_estop_log_time).seconds() > 1.0) {   
                    ground_control->addLogLine(LogLevel::WARN, "Estop command sent");
                    last_estop_log_time = now;
                }
            }

            ImGui::SetCursorPos(ImVec2(1440 * scale, 670 * scale));
            widgets.AltitudeTape(1, Info.ground_distance, 0.5f, theme, scale);

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
        int Map_Upanel_x = 1510 * scale;
        int Map_Upanel_y = 80 * scale;
        BeginOverlayPanel(draw_list, "MapUtilsPanel", ImVec2(Map_Upanel_x, Map_Upanel_y), ImVec2(49 * scale, 150 * scale), scale, theme);

        GLuint Plus_Icon = theme ? images.at("plus_white") : images.at("plus");
        if (widgets.CustomButton(draw_list, ImVec2(Map_Upanel_x + 24, Map_Upanel_y + 25 ), "Plus", scale, Plus_Icon, theme, -5, -3)) {
            if (map_zoom < 20) {
                map_zoom += 1;
            }
        }
        GLuint Minus_Icon = theme ? images.at("minus_white") : images.at("minus");
        if (widgets.CustomButton(draw_list, ImVec2(Map_Upanel_x + 24 , Map_Upanel_y + 69 ), "Minus", scale, Minus_Icon, theme, -5, -3)) {
            if (map_zoom > 13) {
                map_zoom -= 1;
            }
        }

        GLuint Switch_Icon = theme ? images.at("switch_white") : images.at("switch");
        if (widgets.CustomButton(draw_list, ImVec2(Map_Upanel_x + 24 , Map_Upanel_y + 113 ), "Switch", scale, Switch_Icon, theme, -5, -3)) {
            sat_map = !sat_map;
        }

        EndOverlayPanel();
        
       

        

        mission_control_bar.Draw(planner, scale, theme,
                                 70 * scale, 70 * scale, 1500 * scale, 800 * scale,
                                 guard_with_checklist);

         // -----------------------------------Control Panel-----------------------------

        BeginOverlayPanel(draw_list, "ControlPanel", ImVec2(90 * scale, 305 * scale), ImVec2(60 * scale, 370 * scale), scale, theme);

        
        GLuint Up_Icon = theme ? images.at("up_white") : images.at("up");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 335 * scale), "Up", scale, Up_Icon, theme, -1, 5)) {
            ground_control->send_command("takeoff", vector<double>{-1.5}); 
            ground_control->addLogLine(LogLevel::INFO,"Takeoff command sent");
        }
        ImGui::PushFont(winInit.getFont(14));
        draw_list->AddText(ImVec2(100 * scale, 362 * scale), colors.white_black(theme), "TakeOff");

        GLuint Goto_Icon = theme ? images.at("goto_white") : images.at("goto");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 405 * scale), "Goto", scale, Goto_Icon, theme, -1, 5)) {
            //ground_control->send_command("set_origin"); //!!! Make the goto function
            //ground_control->addLogLine(LogLevel::INFO, "Goto (something something something) command sent");
        }
        draw_list->AddText(ImVec2(106 * scale, 432 * scale), colors.white_black(theme), "Goto");
        

        GLuint Down_Icon = theme ? images.at("down_white") : images.at("down");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 475 * scale), "Down", scale, Down_Icon, theme, -1, 5)) {
            ground_control->send_command("land");
            ground_control->addLogLine(LogLevel::INFO, "Land command sent");
        }
        draw_list->AddText(ImVec2(107 * scale, 502 * scale), colors.white_black(theme), "Land");

        GLuint Home_Icon = theme ? images.at("home_white") : images.at("home");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 545 * scale), "Home", scale, Home_Icon, theme, -1, 5)) {
            ground_control->send_command("rth"); 
            ground_control->addLogLine(LogLevel::INFO, "Return to home command sent");
        }
        draw_list->AddText(ImVec2(106 * scale, 572 * scale), colors.white_black(theme), "Home");

        GLuint Origin_Icon = theme ? images.at("origin_white") : images.at("origin");
        if (widgets.CustomButton(draw_list, ImVec2(120 * scale, 615 * scale), "Origin", scale, Origin_Icon, theme, -1, 5)) {
            ground_control->send_command("set_origin"); 
            char origin_txt[64];
            snprintf(origin_txt, sizeof(origin_txt), "Origin set at previously: %.2f , %.2f", Info.xyz_pos[0], Info.xyz_pos[1]);
            ground_control->addLogLine(LogLevel::INFO, origin_txt);
        }
        draw_list->AddText(ImVec2(106 * scale, 642 * scale), colors.white_black(theme), "Origin");

        ImGui::PopFont();
        EndOverlayPanel();

        //GLuint Send_Icon = images.at("send");
        //widgets.GotoPanel(ImVec2(70, 880), scale, theme, goto_text);
        //if (widgets.GotoButton(draw_list, ImVec2(390 * scale, 965 * scale), scale, theme, Send_Icon)){
        //    istringstream iss(goto_text);
        //    double x, y, z, yaw;
//
        //    if (iss >> x >> y >> z >> yaw) {
        //        ground_control->send_command("goto", vector<double>{x, y, z}, yaw);
        //        logs.outputlog(ImVec2(500, 880), scale, theme);   // adjust to your actual logging call once that's built
        //    } else {
        //        istringstream iss2(goto_text);
        //        if (iss2 >> x >> y >> z) {
        //            ground_control->send_command("goto", vector<double>{x, y, z}, 0.0);   // or last_yaw, if you track it
        //        } else {
        //            cout << "Invalid input for goto, please enter x y z yaw values" << endl;
        //        }
        //    }
        //} 
        


        //--------------------------------------Information panels---------------------------------------
        info_panels.Battery_Info(scale, theme, Info.battery_values_C, Info.battery_values_M, Info.motor_speed);
        
        info_panels.Position_Info(scale, theme, Info.xyz_pos, Info.tgt_pos, Info.velocity);
        info_panels.GNSS_Info(scale, theme, 
                            Info.RTK_INFO.rtk_accuracy, 
                            Info.RTK_INFO.rtk_accuracy_target, 
                            Info.RTK_INFO.rtk_survey_duration,
                            Info.gps_status);
        info_panels.Probe_Info(scale, theme, Info.probes);
        
        
        info_panels.ResetPanelTracking();

    
        if (logs.outputlog(ImVec2(70, 880), scale, theme, ground_control->getLogLines(), log_filters)) {
            ground_control->clearLog();
        }



        break;
        }
        case 1:
        {
            planner_panel.Draw(scale, theme, static_cast<float>(y_sc));

            // Reuses the 10px rhythm already established between VehicleAndPalettePanel and MissionPlannerPanel.
            const float map_gap = 10.0f * scale;
            const float planner_right_edge = (70.0f + 450.0f) * scale;  // matches DrawTaskList's panel rect
            const float map_x = planner_right_edge + map_gap;
            const float map_right_margin = map_gap;
            const float map_w = std::max(200.0f * scale, static_cast<float>(x_sc) - map_x - map_right_margin);
            if(sat_map){
                BeginFixedPanel("PlannerMapPanel", ImVec2(map_x, 70 * scale), ImVec2(map_w, map_size_x * scale),
                        scale, theme, 0, ImVec2(0, 0));
                ImVec2 planner_map_pos = location.MapWidget(origin_gps.x(), origin_gps.y(), map_w, 800 * scale, scale, map_zoom, placeholderTile, theme);
                const auto [highlighted_top, highlighted_nested] = planner_panel.highlighted_task();
                // Foreground-drawn, so it renders above modals too -- skip while Load/Save is up, or it bleeds through.
                MapTaskClick map_click;
                if (!mission_control_bar.IsDialogOpen()) {
                    map_click = DrawPlanRouteOverlay(location, planner.plan(), origin_gps.x(), origin_gps.y(),
                                         origin_gps.x(), origin_gps.y(), map_zoom,
                                         planner_map_pos, map_w, 800 * scale, 800 * scale, scale, highlighted_top, highlighted_nested);
                    DrawUavPositionMarker(location, Info.gps_status.latitude, Info.gps_status.longitude,
                                          origin_gps.x(), origin_gps.y(), map_zoom,
                                          planner_map_pos, map_w, 800 * scale, 800 * scale, scale);
                }
                if (map_click.clicked) {
                    planner_panel.SelectTask(map_click.top_level_index, map_click.nested_index);
                }
                EndFixedPanel();

                

            }
            else{
                BeginFixedPanel("PlannerSatMapPanel", ImVec2(map_x, 70 * scale), ImVec2(map_w, map_size_x * scale),
                        scale, theme, 0, ImVec2(0, 0));
               location.NoSatMap(Info, map_w, map_size_x * scale, scale, map_zoom, theme); 
               EndFixedPanel();

            }
            float PlannerMap_Upanel_x = map_x + map_w - 60.0f * scale;  
            float PlannerMap_Upanel_y = 70.0f * scale + 10.0f * scale;   

            BeginOverlayPanel(draw_list, "PlannerMapUtilsPanel", ImVec2(PlannerMap_Upanel_x, PlannerMap_Upanel_y), ImVec2(49 * scale, 150 * scale), scale, theme);

            GLuint PlannerPlus_Icon = theme ? images.at("plus_white") : images.at("plus");
            if (widgets.CustomButton(draw_list, ImVec2(PlannerMap_Upanel_x + 24.0f * scale, PlannerMap_Upanel_y + 25.0f * scale), "Plus", scale, PlannerPlus_Icon, theme, -5, -3)) {
                if (map_zoom < 20) {
                    map_zoom += 1;
                }
            }
            GLuint PlannerMinus_Icon = theme ? images.at("minus_white") : images.at("minus");
            if (widgets.CustomButton(draw_list, ImVec2(PlannerMap_Upanel_x + 24.0f * scale, PlannerMap_Upanel_y + 69.0f * scale), "Minus", scale, PlannerMinus_Icon, theme, -5, -3)) {
                if (map_zoom > 13) {
                    map_zoom -= 1;
                }
            }
            GLuint PlannerSwitch_Icon = theme ? images.at("switch_white") : images.at("switch");
            if (widgets.CustomButton(draw_list, ImVec2(PlannerMap_Upanel_x + 24.0f * scale, PlannerMap_Upanel_y + 113.0f * scale), "Switch", scale, PlannerSwitch_Icon, theme, -5, -3)) {
                sat_map = !sat_map;
            }
            EndOverlayPanel();

            // Height fills to the real window edge rather than a fixed 150*scale.
            const float chart_y = 880.0f * scale;
            const float chart_h = std::max(80.0f * scale, static_cast<float>(y_sc) - chart_y - map_gap);
            DrawHeightChart(planner.plan(), ImVec2(map_x, chart_y), ImVec2(map_w, chart_h), scale, theme);
        }
        break;
        case 2:
        {
            
 
        GLuint Camera_Icon = theme ? images.at("camera_white") : images.at("camera");
            if (widgets.CustomButton(draw_list, ImVec2(1200 * scale, 500 * scale), "Camera", scale, Camera_Icon, theme, 2, 5)) {
                camera_stream_requested = !camera_stream_requested;
                ground_control->setCameraStreaming(camera_stream_requested);
            }
         CameraFeedPanel(ImVec2(500 * scale, 500 * scale), scale, theme, camera_texture_);
            //WeatherData data_test;
            //data_test = FetchWeather(Info.gps_status.latitude, Info.gps_status.longitude);
            //cout << data_test.temperature << " Cloud Cover: " << data_test.cloud_cover << " Windspeed: " << data_test.wind_speed_level << endl;
            
        }
        break;
        case 3:
        {
            analyze_panel.Draw(scale, theme, static_cast<float>(x_sc), static_cast<float>(y_sc),
                               location, map_zoom, placeholderTile);
        }
        break;
        default:
            break;
        }
        ImGui::End();
   
        prearm_checklist.Draw(draw_list, ImVec2(450 * scale, 62 * scale), scale, theme,
                               prearm_snapshot, prearm_hard_issues);

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