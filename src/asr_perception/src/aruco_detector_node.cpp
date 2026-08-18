#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <asr_comms/msg/aruco_detections.hpp>

// MATLAB Coder generated — produced by running matlab/generate_code.m
// Build will fail here until generate_code.m has been executed.
#include "detect_aruco_in_camera_frame.h"
#include "detect_aruco_in_camera_frame_initialize.h"
#include "detect_aruco_in_camera_frame_terminate.h"

using ArucoDetections = asr_comms::msg::ArucoDetections;
using Image           = sensor_msgs::msg::Image;

class ArucoDetectorNode : public rclcpp::Node {
public:
    ArucoDetectorNode() : Node("aruco_detector")
    {
        declare_parameter("fx",             425.88);
        declare_parameter("fy",             425.88);
        declare_parameter("cx",             430.51);
        declare_parameter("cy",             238.53);
        declare_parameter("marker_size_mm", 150.0);

        fx_          = get_parameter("fx").as_double();
        fy_          = get_parameter("fy").as_double();
        cx_          = get_parameter("cx").as_double();
        cy_          = get_parameter("cy").as_double();
        marker_size_ = get_parameter("marker_size_mm").as_double();

        detect_aruco_in_camera_frame_initialize();

        // Relative topic — resolves under the node's namespace (e.g. asr/thyra),
        // matching camera_relay's synced colour stream on the same vehicle.
        auto qos = rclcpp::QoS(5).best_effort();
        image_sub_ = create_subscription<Image>(
            "out/cam/synced/color", qos,
            std::bind(&ArucoDetectorNode::on_image, this, std::placeholders::_1));

        det_pub_ = create_publisher<ArucoDetections>(
            "aruco_detector/detections", 10);

        RCLCPP_INFO(get_logger(), "ArUco detector ready (marker %.0f mm)", marker_size_);
    }

    ~ArucoDetectorNode() { detect_aruco_in_camera_frame_terminate(); }

private:
    void on_image(const Image::ConstSharedPtr& msg)
    {
        // Expect RGB8 or BGR8 — MATLAB Coder function takes uint8 H×W×3.
        // The raw data buffer is already contiguous row-major, matching C layout.
        if (msg->encoding != "rgb8" && msg->encoding != "bgr8") {
            RCLCPP_WARN_ONCE(get_logger(),
                "Unexpected encoding '%s' — expected rgb8 or bgr8", msg->encoding.c_str());
            return;
        }
        if (msg->height != 480 || msg->width != 640) {
            RCLCPP_WARN_ONCE(get_logger(),
                "Image size %ux%u — codegen expects 480×640", msg->width, msg->height);
            return;
        }

        int    ids[16]    = {};
        float  tvec[3*16] = {};
        float  rvec[3*16] = {};
        int    n          = 0;

        detect_aruco_in_camera_frame(
            msg->data.data(),
            fx_, fy_, cx_, cy_, marker_size_,
            ids, tvec, rvec, &n);

        if (n == 0) return;

        ArucoDetections out;
        out.header         = msg->header;
        out.num_detections = static_cast<int32_t>(n);

        for (int i = 0; i < n; ++i) {
            out.marker_id.push_back(ids[i]);
            out.tvec.push_back(tvec[i*3 + 0]);
            out.tvec.push_back(tvec[i*3 + 1]);
            out.tvec.push_back(tvec[i*3 + 2]);
            out.rvec.push_back(rvec[i*3 + 0]);
            out.rvec.push_back(rvec[i*3 + 1]);
            out.rvec.push_back(rvec[i*3 + 2]);
        }

        det_pub_->publish(out);
    }

    double fx_, fy_, cx_, cy_, marker_size_;

    rclcpp::Subscription<Image>::SharedPtr        image_sub_;
    rclcpp::Publisher<ArucoDetections>::SharedPtr det_pub_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ArucoDetectorNode>());
    rclcpp::shutdown();
    return 0;
}
