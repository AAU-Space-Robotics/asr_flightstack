#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "asr_comms/msg/link_stats.hpp"

#include "simple_writer.hpp"

namespace asr_logger {

// ---------------------------------------------------------------------------

static std::string makeLogFilename(const std::string& dir)
{
    std::filesystem::create_directories(dir);
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream ss;
    ss << dir << "/gcs_"
       << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << ".ulg";
    return ss.str();
}

static uint64_t nowUs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
        .count());
}

// ---------------------------------------------------------------------------
// ULOG data structs
// Fields must be ordered by decreasing type size — no internal padding allowed.
// Tail padding is fine (SimpleWriter copies only the declared field bytes).
// ---------------------------------------------------------------------------

// LinkStats from comms_gcs (1 Hz). LinkStats carries no timestamp of its own,
// so records are stamped with the arrival time.
struct UlogLinkStats {
    uint64_t timestamp;           // us, arrival time
    float    radio_tx_kbps;       // GCS→UAV MAVLink via radio
    float    mavlink_rx_kbps;     // UAV→GCS, radio + WiFi combined
    float    radio_rx_kbps;       // UAV→GCS via radio only
    float    wifi_rx_kbps;        // UAV→GCS via WiFi only
    float    uav_rx_kbps;         // UAV-reported receive rate from GCS
    float    camera_rx_kbps;      // camera JPEG stream (WiFi-only)
    uint32_t radio_rx_msgs;       // unique MAVLink messages first seen on radio
    uint32_t wifi_rx_msgs;        // unique MAVLink messages first seen on WiFi
    uint16_t radio_rxerrors;      // GCS radio RX error count (cumulative)
    uint16_t radio_fixed;         // GCS radio corrected-packet count (cumulative)
    uint16_t uav_rxerrors;        // UAV radio RX error count (cumulative)
    uint8_t  mavlink_connected;
    uint8_t  wifi_connected;
    uint8_t  camera_streaming;
    uint8_t  radio_ok;
    uint8_t  radio_rssi;
    uint8_t  radio_remrssi;
    uint8_t  radio_noise;
    uint8_t  radio_remnoise;
    uint8_t  radio_txbuf;         // GCS radio TX buffer remaining [%] — 0 = saturated
    uint8_t  signal_quality;      // LinkStats::QUALITY_*
    uint8_t  uav_radio_ok;
    uint8_t  uav_rssi;
    uint8_t  uav_noise;
    uint8_t  uav_txbuf;           // UAV radio TX buffer remaining [%] — 0 = saturated
};

// ---------------------------------------------------------------------------

class LoggerGcsNode : public rclcpp::Node {
public:
    LoggerGcsNode()
    : Node("asr_logger_gcs")
    {
        declare_parameter("log_dir",  std::string{"~/gcs_logs"});
        declare_parameter("sys_name", std::string{"asr_gcs"});

        log_dir_  = get_parameter("log_dir").as_string();
        sys_name_ = get_parameter("sys_name").as_string();

        if (!log_dir_.empty() && log_dir_[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) log_dir_ = std::string(home) + log_dir_.substr(1);
        }

        initWriter();

        link_stats_sub_ = create_subscription<asr_comms::msg::LinkStats>(
            "link_stats", rclcpp::QoS(10),
            std::bind(&LoggerGcsNode::onLinkStats, this, std::placeholders::_1));

        flush_timer_ = create_wall_timer(
            std::chrono::seconds(1),
            std::bind(&LoggerGcsNode::onFlushTimer, this));

        RCLCPP_INFO(get_logger(), "GCS logger started");
    }

    ~LoggerGcsNode() override
    {
        if (writer_) {
            writer_->fsync();
        }
    }

private:
    void initWriter()
    {
        using F = ulog_cpp::Field;
        const std::vector<F> link_stats_fields = {
            {"uint64_t", "timestamp"},
            {"float",    "radio_tx_kbps"},   {"float", "mavlink_rx_kbps"},
            {"float",    "radio_rx_kbps"},   {"float", "wifi_rx_kbps"},
            {"float",    "uav_rx_kbps"},     {"float", "camera_rx_kbps"},
            {"uint32_t", "radio_rx_msgs"},   {"uint32_t", "wifi_rx_msgs"},
            {"uint16_t", "radio_rxerrors"},  {"uint16_t", "radio_fixed"},
            {"uint16_t", "uav_rxerrors"},
            {"uint8_t",  "mavlink_connected"}, {"uint8_t", "wifi_connected"},
            {"uint8_t",  "camera_streaming"},  {"uint8_t", "radio_ok"},
            {"uint8_t",  "radio_rssi"},        {"uint8_t", "radio_remrssi"},
            {"uint8_t",  "radio_noise"},       {"uint8_t", "radio_remnoise"},
            {"uint8_t",  "radio_txbuf"},       {"uint8_t", "signal_quality"},
            {"uint8_t",  "uav_radio_ok"},      {"uint8_t", "uav_rssi"},
            {"uint8_t",  "uav_noise"},         {"uint8_t", "uav_txbuf"},
        };

        const std::string path = makeLogFilename(log_dir_);

        writer_ = std::make_unique<ulog_cpp::SimpleWriter>(path, nowUs());
        writer_->writeInfo("sys_name", sys_name_);

        writer_->writeMessageFormat("link_stats", link_stats_fields);
        writer_->headerComplete();
        id_link_stats_ = writer_->writeAddLoggedMessage("link_stats");

        RCLCPP_INFO(get_logger(), "Writing to: %s", path.c_str());
    }

    void onLinkStats(const asr_comms::msg::LinkStats& msg)
    {
        if (!writer_) return;
        UlogLinkStats frame{
            .timestamp         = static_cast<uint64_t>(get_clock()->now().nanoseconds() / 1000),
            .radio_tx_kbps     = msg.radio_tx_kbps,
            .mavlink_rx_kbps   = msg.mavlink_rx_kbps,
            .radio_rx_kbps     = msg.radio_rx_kbps,
            .wifi_rx_kbps      = msg.wifi_rx_kbps,
            .uav_rx_kbps       = msg.uav_rx_kbps,
            .camera_rx_kbps    = msg.camera_rx_kbps,
            .radio_rx_msgs     = msg.radio_rx_msgs,
            .wifi_rx_msgs      = msg.wifi_rx_msgs,
            .radio_rxerrors    = msg.radio_rxerrors,
            .radio_fixed       = msg.radio_fixed,
            .uav_rxerrors      = msg.uav_rxerrors,
            .mavlink_connected = msg.mavlink_connected ? uint8_t{1} : uint8_t{0},
            .wifi_connected    = msg.wifi_connected    ? uint8_t{1} : uint8_t{0},
            .camera_streaming  = msg.camera_streaming  ? uint8_t{1} : uint8_t{0},
            .radio_ok          = msg.radio_ok          ? uint8_t{1} : uint8_t{0},
            .radio_rssi        = msg.radio_rssi,
            .radio_remrssi     = msg.radio_remrssi,
            .radio_noise       = msg.radio_noise,
            .radio_remnoise    = msg.radio_remnoise,
            .radio_txbuf       = msg.radio_txbuf,
            .signal_quality    = msg.signal_quality,
            .uav_radio_ok      = msg.uav_radio_ok      ? uint8_t{1} : uint8_t{0},
            .uav_rssi          = msg.uav_rssi,
            .uav_noise         = msg.uav_noise,
            .uav_txbuf         = msg.uav_txbuf,
        };
        writer_->writeData(id_link_stats_, frame);
    }

    void onFlushTimer()
    {
        if (writer_) {
            writer_->fsync();
        }
    }

    std::unique_ptr<ulog_cpp::SimpleWriter> writer_;
    std::string log_dir_;
    std::string sys_name_;

    uint16_t id_link_stats_{};

    rclcpp::Subscription<asr_comms::msg::LinkStats>::SharedPtr link_stats_sub_;
    rclcpp::TimerBase::SharedPtr flush_timer_;
};

}  // namespace asr_logger

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<asr_logger::LoggerGcsNode>());
    rclcpp::shutdown();
    return 0;
}
