#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/u_int8_multi_array.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <asr_comms/msg/camera_stream_request.hpp>
#include <asr_comms/msg/telemetry_position.hpp>
#include <asr_comms/msg/telemetry_attitude.hpp>
#include <asr_comms/msg/telemetry_battery.hpp>
#include <asr_comms/msg/telemetry_gps.hpp>
#include <asr_comms/msg/telemetry_status.hpp>
#include <asr_comms/msg/uav_command.hpp>
#include <asr_comms/msg/command_ack.hpp>
#include <asr_comms/msg/gcs_heartbeat.hpp>
#include <asr_comms/msg/manual_control_input.hpp>
#include <asr_comms/msg/servo_command.hpp>
#include <asr_comms/msg/link_stats.hpp>
#include <px4_msgs/msg/distance_sensor.hpp>

#include "camera_protocol.h"
#include "common/mavlink.h"
#include "dedup.h"
#include "mission_protocol.h"
#include "transport.h"
#include "udp_socket.h"

class SerialPort;

// Runs on the GCS machine. Transport is either UDP or a serial SiK radio.
// Sends to drone:      heartbeat, RTK corrections.
// Receives from drone: heartbeat + telemetry published on in/telemetry/* topics.
class CommsGcs : public rclcpp::Node {
public:
    CommsGcs();
    ~CommsGcs();

private:
    // Receive path
    void recv_loop();
    void wifi_recv_loop();
    bool reconnect_serial();
    void handle_message(const mavlink_message_t& msg);

    // Send path
    void send_mavlink(mavlink_message_t& msg, LinkTarget target = LinkTarget::Both);
    void send_heartbeat();
    void send_peer_beacon();
    void send_rtcm(const std_msgs::msg::UInt8MultiArray::SharedPtr msg);
    void on_uav_command(const asr_comms::msg::UAVCommand::SharedPtr msg);
    void on_gcs_heartbeat(const asr_comms::msg::GcsHeartbeat::SharedPtr msg);
    void on_manual_input(const asr_comms::msg::ManualControlInput::SharedPtr msg);
    void on_servo_command(const asr_comms::msg::ServoCommand::SharedPtr msg);
    void publish_link_stats();

    // Mission bridge — mirror image of comms_uav's: reassembles
    // validate/status blobs from the UAV and republishes them on
    // out/mission_* for a future GCS UI; fragments and forwards
    // in/mission_upload / in/mission_start / in/mission_abort to the UAV
    // WiFi-only. Upload/start/abort are resent on a timeout if their
    // implicit ack (a ValidateResult for upload, any Status for start or
    // abort) never arrives -- see check_mission_retries.
    void handle_mission_v2_extension(const mavlink_v2_extension_t& ext);
    void send_mission_blob(uint16_t message_type, uint32_t& transfer_id, const std::string& blob);
    void on_mission_upload(const std_msgs::msg::String::SharedPtr msg);
    void on_mission_start(const std_msgs::msg::String::SharedPtr msg);
    void on_mission_abort(const std_msgs::msg::String::SharedPtr msg);
    void check_mission_retries();

    // Camera streaming
    void camera_recv_loop();
    void on_camera_stream_request(const asr_comms::msg::CameraStreamRequest::SharedPtr msg);

    std::unique_ptr<ITransport> transport_;
    SerialPort*                 serial_{nullptr};  // aliases transport_ when serial; used by recv_loop to reopen after unplug
    std::string                 serial_param_;     // configured device path or "auto"
    std::unique_ptr<UdpSocket>  wifi_transport_;  // server-mode UDP, nullptr until WiFi ready
    uint32_t                    beacon_tick_{0};  // counts 1 Hz timer fires for keepalive pacing

    uint8_t system_id_   {255};
    uint8_t component_id_{  0};

    // Receive side
    std::thread       recv_thread_;
    std::thread       wifi_recv_thread_;
    std::atomic<bool> running_{true};
    std::mutex        recv_mutex_;   // serialises handle_message across both recv threads
    DedupFilter       dedup_;

    // WiFi peer-beacon config (GCS advertises itself; UAV IP is learned via recvfrom)
    uint32_t wifi_ip_{0};    // GCS WiFi IP in network byte order, 0 = no route detected
    uint16_t wifi_port_{0};  // GCS WiFi UDP bind port

    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr              uav_heartbeat_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryPosition>::SharedPtr position_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryAttitude>::SharedPtr attitude_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryBattery>::SharedPtr  battery_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryBattery>::SharedPtr  battery2_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryGPS>::SharedPtr      gps_pub_;
    rclcpp::Publisher<asr_comms::msg::TelemetryStatus>::SharedPtr   status_pub_;
    rclcpp::Publisher<px4_msgs::msg::DistanceSensor>::SharedPtr     distance_pub_;

    // Send side
    rclcpp::TimerBase::SharedPtr heartbeat_timer_;
    rclcpp::TimerBase::SharedPtr stats_timer_;
    rclcpp::TimerBase::SharedPtr beacon_timer_;
    rclcpp::Publisher<asr_comms::msg::LinkStats>::SharedPtr link_stats_pub_;
    std::atomic<size_t>   tx_bytes_{0};
    std::atomic<size_t>   radio_rx_bytes_{0};
    std::atomic<size_t>   wifi_rx_bytes_{0};
    std::atomic<uint32_t> radio_rx_msgs_{0};   // unique msgs first seen on radio path
    std::atomic<uint32_t> wifi_rx_msgs_{0};    // unique msgs first seen on WiFi path
    std::atomic<uint64_t> last_rx_ns_{0};      // ns timestamp of last received byte (either path)

    std::atomic<float>    uav_rx_kbps_{0.0f};  // reported by UAV via NAMED_VALUE_FLOAT

    // UAV-side radio stats forwarded from comms_uav via NAMED_VALUE_FLOAT
    std::atomic<uint8_t>  uav_radio_txbuf_{255};
    std::atomic<uint8_t>  uav_radio_rssi_{255};
    std::atomic<uint8_t>  uav_radio_noise_{255};
    std::atomic<uint16_t> uav_radio_rxerrors_{0};
    std::atomic<uint64_t> last_uav_radio_ns_{0};

    // SiK RADIO_STATUS fields (MAVLINK_MSG_ID_RADIO_STATUS = 109)
    std::atomic<uint8_t>  radio_rssi_{255};
    std::atomic<uint8_t>  radio_remrssi_{255};
    std::atomic<uint8_t>  radio_noise_{255};
    std::atomic<uint8_t>  radio_remnoise_{255};
    std::atomic<uint8_t>  radio_txbuf_{0};
    std::atomic<uint16_t> radio_rxerrors_{0};
    std::atomic<uint16_t> radio_fixed_{0};
    std::atomic<uint64_t> last_radio_ns_{0}; // ns timestamp of last RADIO_STATUS
    rclcpp::Subscription<std_msgs::msg::UInt8MultiArray>::SharedPtr         rtcm_sub_;
    rclcpp::Subscription<asr_comms::msg::UAVCommand>::SharedPtr             uav_command_sub_;
    rclcpp::Subscription<asr_comms::msg::GcsHeartbeat>::SharedPtr          gcs_heartbeat_sub_;
    rclcpp::Subscription<asr_comms::msg::ManualControlInput>::SharedPtr    manual_input_sub_;
    rclcpp::Subscription<asr_comms::msg::ServoCommand>::SharedPtr          servo_command_sub_;
    rclcpp::Publisher<asr_comms::msg::CommandAck>::SharedPtr               command_ack_pub_;
    rclcpp::Subscription<asr_comms::msg::CameraStreamRequest>::SharedPtr   camera_stream_sub_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr        camera_pub_;

    // Camera receive socket and thread (WiFi-only)
    uint16_t                   camera_port_{5600};
    std::unique_ptr<UdpSocket> camera_transport_;
    std::thread                camera_recv_thread_;
    std::atomic<bool>          camera_streaming_{false};  // tracks start/stop request state
    std::atomic<size_t>        camera_rx_bytes_{0};       // bytes received in camera_recv_loop

    // Frame reassembly
    struct CameraFrag { std::vector<uint8_t> data; bool received{false}; };
    struct {
        uint32_t              frame_id{UINT32_MAX};
        uint32_t              frame_size{0};
        uint16_t              total_frags{0};
        uint16_t              received_count{0};
        std::vector<CameraFrag> frags;
    } camera_assembler_;
    std::mutex camera_assembler_mutex_;
    uint8_t  rtcm_seq_{0};
    int8_t   gcs_nominal_{1};          // latest value from in/gcs_heartbeat, sent in MAVLink heartbeat
    std::string pending_command_type_;
    std::chrono::steady_clock::time_point last_command_sent_{};

    // Mission bridge (see handle_mission_v2_extension / send_mission_blob).
    MissionReassembler validate_reassembler_;
    MissionReassembler status_reassembler_;
    uint32_t upload_transfer_id_{0};  // outgoing to UAV, incremented per send
    uint32_t start_transfer_id_{0};
    uint32_t abort_transfer_id_{0};
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    mission_validate_pub_;  // -> future GCS UI
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr    mission_status_pub_;    // -> future GCS UI
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_upload_sub_;    // <- future GCS UI
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_start_sub_;     // <- future GCS UI
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_abort_sub_;     // <- future GCS UI

    // Retry state for upload/start/abort (see check_mission_retries). One
    // in-flight attempt tracked per kind; a new send of that kind from the
    // GCS UI replaces whatever was pending.
    struct PendingMissionSend {
        bool        active{false};
        std::string blob;      // resent verbatim under a new transfer_id each retry
        int         attempts{0};
        std::chrono::steady_clock::time_point sent_at{};
    };
    PendingMissionSend pending_upload_;
    PendingMissionSend pending_start_;
    PendingMissionSend pending_abort_;
    rclcpp::TimerBase::SharedPtr mission_retry_timer_;
    static constexpr int MISSION_MAX_RETRIES = 5;
    static constexpr std::chrono::milliseconds MISSION_RETRY_TIMEOUT{2000};

    static constexpr uint16_t ASR_MSG_TELEMETRY_STATUS = 0x9001u;
    static constexpr uint16_t ASR_MSG_SERVO_COMMAND    = 0x9002u;
    static constexpr uint16_t ASR_MSG_PEER_BEACON      = 0x9003u;
    static constexpr uint16_t ASR_MSG_MISSION_UPLOAD   = 0x9004u;
    static constexpr uint16_t ASR_MSG_MISSION_VALIDATE = 0x9005u;
    static constexpr uint16_t ASR_MSG_MISSION_START    = 0x9006u;
    static constexpr uint16_t ASR_MSG_MISSION_STATUS   = 0x9007u;
    static constexpr uint16_t ASR_MSG_MISSION_ABORT    = 0x9008u;

    // ASR custom MAVLink command IDs (local experiment range ≥ 32768)
    static constexpr uint16_t ASR_CMD_GOTO              = 32768u;
    static constexpr uint16_t ASR_CMD_MANUAL            = 32769u;
    static constexpr uint16_t ASR_CMD_MANUAL_AIDED      = 32770u;
    static constexpr uint16_t ASR_CMD_SET_ORIGIN        = 32771u;
    static constexpr uint16_t ASR_CMD_SET_LINEAR_SPEED  = 32772u;
    static constexpr uint16_t ASR_CMD_SET_ANGULAR_SPEED = 32773u;
    static constexpr uint16_t ASR_CMD_SPIN              = 32774u;
    static constexpr uint16_t ASR_CMD_ELAND             = 32775u;
    static constexpr uint16_t ASR_CMD_VELOCITY          = 32776u;
    static constexpr uint16_t ASR_CMD_MULTI_WAYPOINT    = 32777u;
};
