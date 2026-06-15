#include <rclcpp/rclcpp.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <asr_comms/msg/probe_detections.hpp>
#include <asr_comms/msg/probe_locations.hpp>
#include <asr_comms/msg/aruco_detections.hpp>
#include <asr_comms/msg/aruco_locations.hpp>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "kalman_track.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <vector>

using ProbeDetections = asr_comms::msg::ProbeDetections;
using ProbeLocations  = asr_comms::msg::ProbeLocations;
using ArucoDetections = asr_comms::msg::ArucoDetections;
using ArucoLocations  = asr_comms::msg::ArucoLocations;
using Image           = sensor_msgs::msg::Image;
using PoseStamped     = geometry_msgs::msg::PoseStamped;

// ---------------------------------------------------------------------------
// Rodrigues helpers
// ---------------------------------------------------------------------------
static Eigen::Matrix3d rodrigues(const Eigen::Vector3d& v)
{
    const double theta = v.norm();
    if (theta < 1e-10) return Eigen::Matrix3d::Identity();
    return Eigen::AngleAxisd(theta, v / theta).toRotationMatrix();
}

static Eigen::Vector3d to_rvec(const Eigen::Matrix3d& R)
{
    const Eigen::AngleAxisd aa(R);
    return aa.axis() * aa.angle();
}

// ---------------------------------------------------------------------------
// Filter matrices — ProbeTrack (6-state, 3-measurement)
// State:       [px, py, pz, bx, by, bz]
// Measurement: z = [I | I] * x  (measured position = true position + bias)
// ---------------------------------------------------------------------------
static Eigen::Matrix<double, 3, 6> probe_H()
{
    Eigen::Matrix<double, 3, 6> H = Eigen::Matrix<double, 3, 6>::Zero();
    H.leftCols<3>()  = Eigen::Matrix3d::Identity();
    H.rightCols<3>() = Eigen::Matrix3d::Identity();
    return H;
}

static Eigen::Matrix<double, 6, 6> probe_Q()
{
    constexpr double kPos  = 0.01 * 0.01;
    constexpr double kBias = 0.001 * 0.001;
    Eigen::Matrix<double, 6, 6> Q = Eigen::Matrix<double, 6, 6>::Zero();
    Q.topLeftCorner<3, 3>()     = kPos  * Eigen::Matrix3d::Identity();
    Q.bottomRightCorner<3, 3>() = kBias * Eigen::Matrix3d::Identity();
    return Q;
}

static Eigen::Matrix<double, 3, 3> probe_R(double confidence = 1.0)
{
    constexpr double kSigma = 0.05 * 0.05;
    return (kSigma / confidence) * Eigen::Matrix3d::Identity();
}

// ---------------------------------------------------------------------------
// Filter matrices — ArucoTrack (9-state, 6-measurement)
// State:       [px, py, pz, bx, by, bz, rx, ry, rz]  (world frame)
// Measurement: z = [[I|I|0]; [0|0|I]] * x
//              rows 0-2: measured position = true position + bias
//              rows 3-5: measured orientation (rvec, no bias term)
// ---------------------------------------------------------------------------
static Eigen::Matrix<double, 6, 9> aruco_H()
{
    Eigen::Matrix<double, 6, 9> H = Eigen::Matrix<double, 6, 9>::Zero();
    H.block<3, 3>(0, 0) = Eigen::Matrix3d::Identity();  // position
    H.block<3, 3>(0, 3) = Eigen::Matrix3d::Identity();  // bias
    H.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity();  // orientation
    return H;
}

static Eigen::Matrix<double, 9, 9> aruco_Q()
{
    constexpr double kPos  = 0.005 * 0.005;
    constexpr double kBias = 0.0005 * 0.0005;
    constexpr double kRot  = 0.001 * 0.001;   // marker sits still
    Eigen::Matrix<double, 9, 9> Q = Eigen::Matrix<double, 9, 9>::Zero();
    Q.topLeftCorner<3, 3>()     = kPos  * Eigen::Matrix3d::Identity();
    Q.block<3, 3>(3, 3)         = kBias * Eigen::Matrix3d::Identity();
    Q.bottomRightCorner<3, 3>() = kRot  * Eigen::Matrix3d::Identity();
    return Q;
}

static Eigen::Matrix<double, 6, 6> aruco_R()
{
    constexpr double kPos = 0.03 * 0.03;  // ~3 cm PnP position accuracy
    constexpr double kRot = 0.03 * 0.03;  // ~1.7° PnP rotation accuracy
    Eigen::Matrix<double, 6, 6> R = Eigen::Matrix<double, 6, 6>::Zero();
    R.topLeftCorner<3, 3>()     = kPos * Eigen::Matrix3d::Identity();
    R.bottomRightCorner<3, 3>() = kRot * Eigen::Matrix3d::Identity();
    return R;
}

// ---------------------------------------------------------------------------
// Track initializers
// ---------------------------------------------------------------------------
static ProbeTrack init_probe_track(uint32_t id, const Eigen::Vector3d& z, double now_s)
{
    ProbeTrack t;
    t.id             = id;
    t.observations   = 1;
    t.last_update_s  = now_s;
    t.x.head<3>()    = z;
    t.x.tail<3>()    = Eigen::Vector3d::Zero();
    t.P              = Eigen::Matrix<double, 6, 6>::Zero();
    t.P.topLeftCorner<3, 3>()     = (0.05 * 0.05) * Eigen::Matrix3d::Identity();
    t.P.bottomRightCorner<3, 3>() = (0.10 * 0.10) * Eigen::Matrix3d::Identity();
    const auto H = probe_H();
    t.S = H * t.P * H.transpose() + probe_R();
    return t;
}

static ArucoTrack init_aruco_track(int32_t id, const Eigen::Vector3d& pos,
                                    const Eigen::Vector3d& rot, double now_s)
{
    ArucoTrack t;
    t.id              = static_cast<uint32_t>(id);
    t.observations    = 1;
    t.last_update_s   = now_s;
    t.x.head<3>()     = pos;
    t.x.segment<3>(3) = Eigen::Vector3d::Zero();  // bias unknown, start at zero
    t.x.tail<3>()     = rot;
    t.P               = Eigen::Matrix<double, 9, 9>::Zero();
    t.P.topLeftCorner<3, 3>()     = (0.05 * 0.05) * Eigen::Matrix3d::Identity();
    t.P.block<3, 3>(3, 3)         = (0.10 * 0.10) * Eigen::Matrix3d::Identity();
    t.P.bottomRightCorner<3, 3>() = (0.10 * 0.10) * Eigen::Matrix3d::Identity();
    const auto H = aruco_H();
    t.S = H * t.P * H.transpose() + aruco_R();
    return t;
}

// ---------------------------------------------------------------------------
// DetectionFilter node
// ---------------------------------------------------------------------------
class DetectionFilter : public rclcpp::Node {
public:
    DetectionFilter() : Node("detection_filter")
    {
        declare_parameter("fx", 425.88);
        declare_parameter("fy", 425.88);
        declare_parameter("cx", 430.51);
        declare_parameter("cy", 238.53);
        declare_parameter("camera_to_drone_transform", std::vector<double>(16, 0.0));
        declare_parameter("min_observations",          3);
        declare_parameter("merge_distance_m",          0.6);
        declare_parameter("max_distance_m",            4.0);
        declare_parameter("aruco_min_observations",    2);
        declare_parameter("publish_rate_hz",           1.0);

        fx_ = get_parameter("fx").as_double();
        fy_ = get_parameter("fy").as_double();
        cx_ = get_parameter("cx").as_double();
        cy_ = get_parameter("cy").as_double();

        const auto T_flat = get_parameter("camera_to_drone_transform").as_double_array();
        if (T_flat.size() == 16) {
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    T_cam_to_drone_(r, c) = T_flat[r * 4 + c];
        } else {
            RCLCPP_WARN(get_logger(),
                        "camera_to_drone_transform param missing/wrong size — using identity");
            T_cam_to_drone_ = Eigen::Matrix4d::Identity();
        }

        min_obs_probe_  = get_parameter("min_observations").as_int();
        min_obs_aruco_  = get_parameter("aruco_min_observations").as_int();
        max_distance_   = get_parameter("max_distance_m").as_double();
        const double hz = get_parameter("publish_rate_hz").as_double();

        auto qos = rclcpp::QoS(5).best_effort();

        // --- Probe sync: ProbeDetections + depth image + pose (3-topic) ---
        probe_det_sub_.subscribe(this,  "probe_detector/detections", qos.get_rmw_qos_profile());
        depth_sub_.subscribe(this,      "out/cam/synced/depth",       qos.get_rmw_qos_profile());
        probe_pose_sub_.subscribe(this, "out/cam/synced/pose",        qos.get_rmw_qos_profile());

        probe_sync_ = std::make_shared<ProbeSync>(ProbeSyncPolicy(10),
            probe_det_sub_, depth_sub_, probe_pose_sub_);
        probe_sync_->setMaxIntervalDuration(rclcpp::Duration::from_seconds(0.15));
        probe_sync_->registerCallback(
            std::bind(&DetectionFilter::on_probe_data, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // --- ArUco sync: ArucoDetections + pose (2-topic) ---
        aruco_det_sub_.subscribe(this,  "aruco_detector/detections", qos.get_rmw_qos_profile());
        aruco_pose_sub_.subscribe(this, "out/cam/synced/pose",        qos.get_rmw_qos_profile());

        aruco_sync_ = std::make_shared<ArucoSync>(ArucoSyncPolicy(10),
            aruco_det_sub_, aruco_pose_sub_);
        aruco_sync_->setMaxIntervalDuration(rclcpp::Duration::from_seconds(0.15));
        aruco_sync_->registerCallback(
            std::bind(&DetectionFilter::on_aruco_data, this,
                      std::placeholders::_1, std::placeholders::_2));

        // --- Publishers ---
        probe_pub_ = create_publisher<ProbeLocations>("probe_detector/locations", 10);
        aruco_pub_ = create_publisher<ArucoLocations>("aruco_detector/locations", 10);

        // --- 1 Hz publish timer (decoupled from measurement rate) ---
        publish_timer_ = create_wall_timer(
            std::chrono::duration<double>(1.0 / hz),
            std::bind(&DetectionFilter::on_publish_timer, this));

        RCLCPP_INFO(get_logger(), "Detection filter ready (publish %.1f Hz)", hz);
    }

private:
    // -----------------------------------------------------------------------
    // Probe callback: back-project depth → 3-D → world frame → Kalman update
    // -----------------------------------------------------------------------
    void on_probe_data(
        const ProbeDetections::ConstSharedPtr& det,
        const Image::ConstSharedPtr&           depth,
        const PoseStamped::ConstSharedPtr&     pose)
    {
        const double now_s = rclcpp::Time(det->header.stamp).seconds();
        const auto   H     = probe_H();
        const auto   Q     = probe_Q();

        constexpr double CHI2_GATE = 2.79;  // sqrt(chi2_inv(0.95, 3))

        for (uint32_t i = 0; i < det->num_detections; ++i) {
            const Eigen::Vector3d p_cam =
                project_to_3d(det->centroid_x[i], det->centroid_y[i], *depth);
            if (p_cam.z() <= 0.0 || p_cam.norm() > max_distance_) continue;

            const Eigen::Vector4d p_h(p_cam.x(), p_cam.y(), p_cam.z(), 1.0);
            const Eigen::Vector3d p_body  = (T_cam_to_drone_ * p_h).head<3>();
            const Eigen::Vector3d p_world = to_global(p_body, *pose);

            const Eigen::Vector3d         z_meas = p_world;
            const Eigen::Matrix3d         R_meas = probe_R(det->confidence[i]);

            // Nearest-neighbour Mahalanobis association
            double      best_d     = std::numeric_limits<double>::max();
            ProbeTrack* best_track = nullptr;

            for (auto& t : probe_tracks_) {
                const double d = t.mahalanobis(z_meas, H);
                if (d < best_d) { best_d = d; best_track = &t; }
            }

            if (best_d < CHI2_GATE) {
                best_track->predict(Q);
                best_track->update(z_meas, H, R_meas, now_s);
            } else {
                probe_tracks_.push_back(init_probe_track(next_probe_id_++, p_world, now_s));
            }
        }
    }

    // -----------------------------------------------------------------------
    // ArUco callback: transform tvec + rvec to world frame → Kalman update
    // Association is trivial: keyed directly by marker ID.
    // -----------------------------------------------------------------------
    void on_aruco_data(
        const ArucoDetections::ConstSharedPtr& det,
        const PoseStamped::ConstSharedPtr&     pose)
    {
        const double now_s = rclcpp::Time(det->header.stamp).seconds();
        const auto   H     = aruco_H();
        const auto   Q     = aruco_Q();
        const auto   R     = aruco_R();

        // Camera → world rotation (used for rvec transform)
        const auto& o = pose->pose.orientation;
        const Eigen::Quaterniond q_drone(o.w, o.x, o.y, o.z);
        const Eigen::Matrix3d R_cam_to_world =
            q_drone.toRotationMatrix() * T_cam_to_drone_.topLeftCorner<3, 3>();

        for (int32_t i = 0; i < det->num_detections; ++i) {
            // Position: camera frame → body frame → world frame
            const Eigen::Vector3d tvec_cam(
                det->tvec[i * 3 + 0], det->tvec[i * 3 + 1], det->tvec[i * 3 + 2]);
            const Eigen::Vector4d p_h(tvec_cam.x(), tvec_cam.y(), tvec_cam.z(), 1.0);
            const Eigen::Vector3d p_world = to_global((T_cam_to_drone_ * p_h).head<3>(), *pose);

            // Orientation: R_marker_to_cam → R_marker_to_world → rvec
            const Eigen::Vector3d rvec_cam(
                det->rvec[i * 3 + 0], det->rvec[i * 3 + 1], det->rvec[i * 3 + 2]);
            const Eigen::Matrix3d R_marker_to_world = R_cam_to_world * rodrigues(rvec_cam);
            const Eigen::Vector3d rvec_world         = to_rvec(R_marker_to_world);

            // 6-D observation: [position; rotation_vector] in world frame
            Eigen::Matrix<double, 6, 1> z_meas;
            z_meas.head<3>() = p_world;
            z_meas.tail<3>() = rvec_world;

            const int32_t id = det->marker_id[i];
            auto it = aruco_tracks_.find(id);
            if (it == aruco_tracks_.end()) {
                aruco_tracks_.emplace(id, init_aruco_track(id, p_world, rvec_world, now_s));
            } else {
                it->second.predict(Q);
                it->second.update(z_meas, H, R, now_s);
            }
        }
    }

    // -----------------------------------------------------------------------
    // 1 Hz publish timer: prune stale tracks, then publish both outputs
    // -----------------------------------------------------------------------
    void on_publish_timer()
    {
        const auto stamp = now();

        // --- ProbeLocations ---
        ProbeLocations probe_msg;
        probe_msg.header.stamp    = stamp;
        probe_msg.header.frame_id = "world";

        for (const auto& t : probe_tracks_) {
            if (t.observations < min_obs_probe_) continue;
            const auto pos = t.position();
            const auto std = t.position_std();
            probe_msg.positions.push_back(static_cast<float>(pos.x()));
            probe_msg.positions.push_back(static_cast<float>(pos.y()));
            probe_msg.positions.push_back(static_cast<float>(pos.z()));
            probe_msg.uncertainty.push_back(static_cast<float>(std.x()));
            probe_msg.uncertainty.push_back(static_cast<float>(std.y()));
            probe_msg.uncertainty.push_back(static_cast<float>(std.z()));
            ++probe_msg.num_probes;
        }
        probe_pub_->publish(probe_msg);

        // --- ArucoLocations ---
        ArucoLocations aruco_msg;
        aruco_msg.header.stamp    = stamp;
        aruco_msg.header.frame_id = "world";

        for (const auto& [id, t] : aruco_tracks_) {
            if (t.observations < min_obs_aruco_) continue;
            const Eigen::Vector3d pos  = t.position();
            const Eigen::Vector3d rvec = t.x.tail<3>();
            const Eigen::Vector3d std  = t.position_std();

            // Convert rvec → quaternion for the message
            const Eigen::Quaterniond q(rodrigues(rvec));

            aruco_msg.marker_id.push_back(id);
            aruco_msg.position.push_back(static_cast<float>(pos.x()));
            aruco_msg.position.push_back(static_cast<float>(pos.y()));
            aruco_msg.position.push_back(static_cast<float>(pos.z()));
            aruco_msg.orientation.push_back(static_cast<float>(q.w()));
            aruco_msg.orientation.push_back(static_cast<float>(q.x()));
            aruco_msg.orientation.push_back(static_cast<float>(q.y()));
            aruco_msg.orientation.push_back(static_cast<float>(q.z()));
            aruco_msg.uncertainty.push_back(static_cast<float>(std.x()));
            aruco_msg.uncertainty.push_back(static_cast<float>(std.y()));
            aruco_msg.uncertainty.push_back(static_cast<float>(std.z()));
            ++aruco_msg.num_markers;
        }
        aruco_pub_->publish(aruco_msg);
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    Eigen::Vector3d project_to_3d(float u, float v, const Image& depth_img) const
    {
        const int iu = static_cast<int>(std::round(u));
        const int iv = static_cast<int>(std::round(v));
        if (iu < 0 || iv < 0 ||
            iu >= static_cast<int>(depth_img.width) ||
            iv >= static_cast<int>(depth_img.height))
            return Eigen::Vector3d::Zero();

        const uint16_t d_mm = *reinterpret_cast<const uint16_t*>(
            depth_img.data.data() + iv * depth_img.step + iu * sizeof(uint16_t));
        if (d_mm == 0) return Eigen::Vector3d::Zero();

        const double z = d_mm / 1000.0;
        return { (u - cx_) * z / fx_, (v - cy_) * z / fy_, z };
    }

    Eigen::Vector3d to_global(const Eigen::Vector3d& p_body,
                               const PoseStamped&      pose) const
    {
        const auto& o = pose.pose.orientation;
        const auto& t = pose.pose.position;
        const Eigen::Quaterniond q(o.w, o.x, o.y, o.z);
        return q * p_body + Eigen::Vector3d(t.x, t.y, t.z);
    }

    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------
    using ProbeSyncPolicy = message_filters::sync_policies::ApproximateTime<
        ProbeDetections, Image, PoseStamped>;
    using ProbeSync = message_filters::Synchronizer<ProbeSyncPolicy>;

    using ArucoSyncPolicy = message_filters::sync_policies::ApproximateTime<
        ArucoDetections, PoseStamped>;
    using ArucoSync = message_filters::Synchronizer<ArucoSyncPolicy>;

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------
    double          fx_, fy_, cx_, cy_;
    Eigen::Matrix4d T_cam_to_drone_;
    int             min_obs_probe_;
    int             min_obs_aruco_;
    double          max_distance_;

    std::vector<ProbeTrack>       probe_tracks_;
    std::map<int32_t, ArucoTrack> aruco_tracks_;
    uint32_t                      next_probe_id_{0};

    message_filters::Subscriber<ProbeDetections> probe_det_sub_;
    message_filters::Subscriber<Image>           depth_sub_;
    message_filters::Subscriber<PoseStamped>     probe_pose_sub_;
    std::shared_ptr<ProbeSync>                   probe_sync_;

    message_filters::Subscriber<ArucoDetections> aruco_det_sub_;
    message_filters::Subscriber<PoseStamped>     aruco_pose_sub_;
    std::shared_ptr<ArucoSync>                   aruco_sync_;

    rclcpp::Publisher<ProbeLocations>::SharedPtr  probe_pub_;
    rclcpp::Publisher<ArucoLocations>::SharedPtr  aruco_pub_;
    rclcpp::TimerBase::SharedPtr                  publish_timer_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DetectionFilter>());
    rclcpp::shutdown();
    return 0;
}
