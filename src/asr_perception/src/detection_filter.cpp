#include <rclcpp/rclcpp.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <asr_comms/msg/probe_detections.hpp>
#include <asr_comms/msg/probe_locations.hpp>
#include <asr_comms/msg/aruco_detections.hpp>
#include <asr_comms/msg/aruco_locations.hpp>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "kalman_track.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

using ProbeDetections = asr_comms::msg::ProbeDetections;
using ProbeLocations  = asr_comms::msg::ProbeLocations;
using ArucoDetections = asr_comms::msg::ArucoDetections;
using ArucoLocations  = asr_comms::msg::ArucoLocations;
using Image           = sensor_msgs::msg::Image;
using PoseCovStamped  = geometry_msgs::msg::PoseWithCovarianceStamped;

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

// Skew-symmetric matrix [v]ₓ such that [v]ₓ·w = v × w.
static Eigen::Matrix3d skew(const Eigen::Vector3d& v)
{
    Eigen::Matrix3d S;
    S <<      0.0, -v.z(),  v.y(),
          v.z(),     0.0, -v.x(),
         -v.y(),  v.x(),     0.0;
    return S;
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

// Camera-frame stereo measurement noise, modelled as a DIAGONAL (anisotropic)
// covariance because the error is dominated by the optical (Z/depth) axis:
//   lateral X,Y : a centroid error of σ_px pixels maps to (z/f)·σ_px metres
//                 → variance grows with z².
//   depth   Z   : a disparity error σ_d maps to (z²/(fx·b))·σ_d metres
//                 → variance grows with z⁴ (the steep term).
// The Z axis is the camera optical axis; probe_R() rotates this into world.
//
// CALIBRATION: kDispVar2 is the assumed disparity variance in px². 0.05 ≈
// 0.22 px σ → ~6.5 cm depth-σ at 2.5 m: realistic for a real stereo matcher,
// a touch conservative vs the sim's idealised 0.0015·z² (≈0.03 px). Note the
// 5 cm kFloor dominates below ~2.5 m, so it is the next knob to lower for a
// tighter R. Tune against NIS; promote to ROS params for live tuning.
static Eigen::Matrix3d probe_R_cam(double depth, double fx, double fy, double baseline)
{
    constexpr double kDispVar2 = 0.05;         // σ_disparity² [px²]  (depth axis)
    constexpr double kPxVar2   = 1.0;          // σ_centroid²  [px²]  (lateral axes)
    constexpr double kFloor    = 0.05 * 0.05;  // per-axis variance floor [m²]

    const double z2 = depth * depth;
    const double fb = fx * baseline;

    Eigen::Matrix3d R = Eigen::Matrix3d::Zero();
    R(0, 0) = (z2 / (fx * fx)) * kPxVar2 + kFloor;          // X lateral
    R(1, 1) = (z2 / (fy * fy)) * kPxVar2 + kFloor;          // Y lateral
    R(2, 2) = (z2 * z2) / (fb * fb) * kDispVar2 + kFloor;   // Z depth
    return R;
}

// Full WORLD-FRAME measurement covariance for a probe detection — the R that
// feeds the Kalman update. Sum of three independent error sources:
//   Rcw·R_cam·Rcwᵀ   : camera stereo noise (probe_R_cam) rotated into world
//   R_pos            : vehicle position uncertainty (adds directly)
//   [L]ₓ·Σ_att·[L]ₓᵀ : vehicle attitude uncertainty through lever arm L
//                      (world offset dronetoprobe; this term grows with range)
static Eigen::Matrix3d probe_R(double depth, double fx, double fy, double baseline,
                               const Eigen::Matrix3d& Rcw,
                               const Eigen::Vector3d& L,
                               const Eigen::Matrix3d& R_pos,
                               const Eigen::Matrix3d& Sigma_att)
{
    const Eigen::Matrix3d R_cam = probe_R_cam(depth, fx, fy, baseline);
    const Eigen::Matrix3d S_L   = skew(L);
    return Rcw * R_cam * Rcw.transpose() // camera stereo noise rotated into world
         + R_pos // vehicle position uncertainty
         + S_L * Sigma_att * S_L.transpose(); // Vehicle attitude uncertainty through lever arm
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

// Camera-frame PnP measurement noise (constant): position + orientation blocks.
static Eigen::Matrix<double, 6, 6> aruco_R_cam()
{
    constexpr double kPos = 0.03 * 0.03;  // ~3 cm PnP position accuracy
    constexpr double kRot = 0.03 * 0.03;  // ~1.7° PnP rotation accuracy
    Eigen::Matrix<double, 6, 6> R = Eigen::Matrix<double, 6, 6>::Zero();
    R.topLeftCorner<3, 3>()     = kPos * Eigen::Matrix3d::Identity();
    R.bottomRightCorner<3, 3>() = kRot * Eigen::Matrix3d::Identity();
    return R;
}

// Full WORLD-FRAME 6x6 measurement covariance for an ArUco detection — the R
// that feeds the Kalman update. The position block mirrors probe_R, but the
// marker also measures ORIENTATION, which changes how attitude error enters:
//
//   pos–pos : Rcw·R_pos_cam·Rcwᵀ + R_pos + [L]ₓ·Σ_att·[L]ₓᵀ
//             (rotated PnP position noise + vehicle position + lever arm)
//   rot–rot : R_rot_cam + Σ_att
//             a vehicle attitude error rotates the measured marker orientation
//             ONE-FOR-ONE, so Σ_att adds DIRECTLY — no lever arm, and position
//             noise R_pos does NOT enter here. (The PnP rotation noise is
//             isotropic, hence frame-invariant, so it is added un-rotated.)
//   pos–rot : −[L]ₓ·Σ_att
//             the SAME attitude error δθ drives both the position error (via the
//             lever arm) and the orientation error, so the two are correlated.
//             Drop these two off-diagonal blocks to decouple them in the filter.
static Eigen::Matrix<double, 6, 6> aruco_R(const Eigen::Matrix3d& Rcw,
                                           const Eigen::Vector3d& L,
                                           const Eigen::Matrix3d& R_pos,
                                           const Eigen::Matrix3d& Sigma_att)
{
    const Eigen::Matrix<double, 6, 6> Rc = aruco_R_cam();
    const Eigen::Matrix3d S_L = skew(L);

    Eigen::Matrix<double, 6, 6> R = Eigen::Matrix<double, 6, 6>::Zero();
    R.topLeftCorner<3, 3>()     = Rcw * Rc.topLeftCorner<3, 3>() * Rcw.transpose() // camera stereo noise rotated into world
                                + R_pos // vehicle position uncertainty
                                + S_L * Sigma_att * S_L.transpose(); // Vehicle attitude uncertainty through lever arm
    R.bottomRightCorner<3, 3>() = Rc.bottomRightCorner<3, 3>() + Sigma_att; // camera PnP rotation noise + vehicle attitude uncertainty
    const Eigen::Matrix3d cross = -S_L * Sigma_att;   // position ↔ orientation
    R.topRightCorner<3, 3>()    = cross;
    R.bottomLeftCorner<3, 3>()  = cross.transpose();
    return R;
}

// ---------------------------------------------------------------------------
// Vehicle position covariance (world/NED frame) from a pose message's 6x6.
// Covariance is row-major [x, y, z, roll, pitch, yaw]; we read the full 3x3
// position block so any cross-terms the estimator reports are preserved.
// The orientation block is carried in the message but not yet consumed.
// ---------------------------------------------------------------------------
static Eigen::Matrix3d pose_position_cov(const PoseCovStamped& pose)
{
    Eigen::Matrix3d C;
    const auto& c = pose.pose.covariance;
    for (int r = 0; r < 3; ++r)
        for (int k = 0; k < 3; ++k)
            C(r, k) = c[r * 6 + k];
    return C;
}

// Vehicle attitude covariance (world frame, rad²) — the 3x3 orientation block
// (rows/cols 3..5 = roll, pitch, yaw) of the pose message's 6x6 covariance.
static Eigen::Matrix3d pose_orientation_cov(const PoseCovStamped& pose)
{
    Eigen::Matrix3d C;
    const auto& c = pose.pose.covariance;
    for (int r = 0; r < 3; ++r)
        for (int k = 0; k < 3; ++k)
            C(r, k) = c[(r + 3) * 6 + (k + 3)];
    return C;
}

// ---------------------------------------------------------------------------
// Track initializers
// ---------------------------------------------------------------------------
static ProbeTrack init_probe_track(uint32_t id, const Eigen::Vector3d& z, double now_s,
                                    const Eigen::Matrix3d& R_meas)
{
    ProbeTrack t;
    t.id             = id;
    t.observations   = 1;
    t.last_update_s  = now_s;
    t.x.head<3>()    = z;
    t.x.tail<3>()    = Eigen::Vector3d::Zero();
    t.P              = Eigen::Matrix<double, 6, 6>::Zero();
    t.P.topLeftCorner<3, 3>()     = R_meas;  // seed from world-frame measurement noise
    t.P.bottomRightCorner<3, 3>() = (0.10 * 0.10) * Eigen::Matrix3d::Identity();
    const auto H = probe_H();
    t.S = H * t.P * H.transpose() + R_meas;
    return t;
}

// Covariance-weighted (information-form) fusion of probe track `b` into `a`.
// Each detection is gated to exactly one track, so two tracks for the same probe
// were built from DISJOINT measurement sets and are independent — information
// addition is then the correct combination of the two estimates.
static void fuse_probe_tracks(ProbeTrack& a, const ProbeTrack& b)
{
    const auto H = probe_H();
    // Recover the measurement noise behind a's cached innovation covariance
    // (S = H·P·Hᵀ + R) so the gate stays consistent until the next update()
    // rebuilds S from scratch. P here is post-update, so this slightly
    // over-estimates R — a marginally looser gate, which is harmless.
    const Eigen::Matrix3d R = a.S - H * a.P * H.transpose();

    const Eigen::Matrix<double, 6, 6> Ia = a.P.inverse();
    const Eigen::Matrix<double, 6, 6> Ib = b.P.inverse();
    const Eigen::Matrix<double, 6, 6> P  = (Ia + Ib).inverse();

    a.x = P * (Ia * a.x + Ib * b.x);
    a.P = P;
    a.S = H * a.P * H.transpose() + R;
    a.observations += b.observations;
    a.last_update_s = std::max(a.last_update_s, b.last_update_s);
    a.id            = std::min(a.id, b.id);   // keep the earlier-established id
}

static ArucoTrack init_aruco_track(int32_t id, const Eigen::Vector3d& pos,
                                    const Eigen::Vector3d& rot, double now_s,
                                    const Eigen::Matrix<double, 6, 6>& R_meas)
{
    ArucoTrack t;
    t.id              = static_cast<uint32_t>(id);
    t.observations    = 1;
    t.last_update_s   = now_s;
    t.x.head<3>()     = pos;
    t.x.segment<3>(3) = Eigen::Vector3d::Zero();  // bias unknown, start at zero
    t.x.tail<3>()     = rot;
    t.P               = Eigen::Matrix<double, 9, 9>::Zero();
    t.P.topLeftCorner<3, 3>()     = R_meas.topLeftCorner<3, 3>();      // position
    t.P.block<3, 3>(3, 3)         = (0.10 * 0.10) * Eigen::Matrix3d::Identity();  // bias prior
    t.P.bottomRightCorner<3, 3>() = R_meas.bottomRightCorner<3, 3>();  // orientation
    const auto H = aruco_H();
    t.S = H * t.P * H.transpose() + R_meas;
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
        declare_parameter("baseline", 0.05);  // for depth variance model
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
        baseline_ = get_parameter("baseline").as_double();

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
        merge_distance_ = get_parameter("merge_distance_m").as_double();
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
    // Probe callback: back-project depth to 3-D to world frame to Kalman update
    void on_probe_data(
        const ProbeDetections::ConstSharedPtr& det,
        const Image::ConstSharedPtr&           depth,
        const PoseCovStamped::ConstSharedPtr&  pose)
    {
        const double now_s = rclcpp::Time(det->header.stamp).seconds();
        const auto   H     = probe_H();
        const auto   Q     = probe_Q();

        constexpr double CHI2_GATE = 2.795;  // sqrt(chi2_inv(0.95, 3))

        // Camera to world rotation and vehicle-position covariance are constant
        const auto& o = pose->pose.pose.orientation;
        const Eigen::Quaterniond q_drone(o.w, o.x, o.y, o.z);
        const Eigen::Matrix3d Rcw = q_drone.toRotationMatrix() * T_cam_to_drone_.topLeftCorner<3, 3>();
        const Eigen::Matrix3d R_pos     = pose_position_cov(*pose);
        const Eigen::Matrix3d Sigma_att = pose_orientation_cov(*pose);

        for (uint32_t i = 0; i < det->num_detections; ++i) {
            // Back-project to camera frame
            const Eigen::Vector3d p_cam = project_to_3d(det->centroid_x[i], det->centroid_y[i], *depth);
            if (p_cam.z() <= 0.0 || p_cam.norm() > max_distance_) continue;

            // Transform to world frame
            const Eigen::Vector4d p_h(p_cam.x(), p_cam.y(), p_cam.z(), 1.0);
            const Eigen::Vector3d p_body  = (T_cam_to_drone_ * p_h).head<3>();
            const Eigen::Vector3d p_world = to_global(p_body, *pose);

            // Full world-frame measurement covariance (assembled in probe_R).
            const Eigen::Vector3d L      = q_drone * p_body;  // lever arm, world frame
            const Eigen::Vector3d z_meas = p_world;
            const Eigen::Matrix3d R_meas = probe_R(p_cam.z(), fx_, fy_, baseline_, Rcw, L, R_pos, Sigma_att);
           
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
                probe_tracks_.push_back(init_probe_track(next_probe_id_++, p_world, now_s, R_meas));
            }
        }

        // Fold together duplicate tracks the gate may have spawned for a single
        // probe under correlated pose drift, so the published count stays honest.
        merge_probe_tracks();
    }

    // Reconcile duplicate probe tracks: any pair whose position estimates lie
    // within merge_distance_ of each other is fused into one. Correlated pose
    // error can drift a measurement past the association gate and spawn a second
    // track for one physical probe; this folds them back together.
    void merge_probe_tracks()
    {
        const double r2 = merge_distance_ * merge_distance_;
        for (std::size_t i = 0; i < probe_tracks_.size(); ++i) {
            for (std::size_t j = i + 1; j < probe_tracks_.size(); ) {
                const double d2 = (probe_tracks_[i].position()
                                 - probe_tracks_[j].position()).squaredNorm();
                if (d2 < r2) {
                    fuse_probe_tracks(probe_tracks_[i], probe_tracks_[j]);
                    probe_tracks_.erase(probe_tracks_.begin() + static_cast<long>(j));
                } else {
                    ++j;
                }
            }
        }
    }

    // ArUco callback: transform tvec + rvec to world frame to Kalman update
    void on_aruco_data(
        const ArucoDetections::ConstSharedPtr& det,
        const PoseCovStamped::ConstSharedPtr&  pose)
    {
        const double now_s = rclcpp::Time(det->header.stamp).seconds();
        const auto   H     = aruco_H();
        const auto   Q     = aruco_Q();

        // Camera to world rotation, plus the vehicle pose covariances that feed into the measurement noise
        const auto& o = pose->pose.pose.orientation;
        const Eigen::Quaterniond q_drone(o.w, o.x, o.y, o.z);
        const Eigen::Matrix3d R_cam_to_world = q_drone.toRotationMatrix() * T_cam_to_drone_.topLeftCorner<3, 3>();
        const Eigen::Matrix3d R_pos     = pose_position_cov(*pose);
        const Eigen::Matrix3d Sigma_att = pose_orientation_cov(*pose);

        for (int32_t i = 0; i < det->num_detections; ++i) {
            // Position: camera frame to body frame to world frame
            const Eigen::Vector3d tvec_cam(det->tvec[i * 3 + 0], det->tvec[i * 3 + 1], det->tvec[i * 3 + 2]); // Decode from PnP output
            const Eigen::Vector4d p_h(tvec_cam.x(), tvec_cam.y(), tvec_cam.z(), 1.0); // homogeneous camera-frame position
            const Eigen::Vector3d p_body  = (T_cam_to_drone_ * p_h).head<3>(); // body frame
            const Eigen::Vector3d p_world = to_global(p_body, *pose);

            // Orientation: R_marker_to_cam to R_marker_to_world to rvec
            const Eigen::Vector3d rvec_cam(det->rvec[i * 3 + 0], det->rvec[i * 3 + 1], det->rvec[i * 3 + 2]); // Decode from PnP output
            const Eigen::Matrix3d R_marker_to_world = R_cam_to_world * rodrigues(rvec_cam);
            const Eigen::Vector3d rvec_world         = to_rvec(R_marker_to_world);

            // 6-D observation: [position; rotation_vector] in world frame
            Eigen::Matrix<double, 6, 1> z_meas;
            z_meas.head<3>() = p_world;
            z_meas.tail<3>() = rvec_world;

            // Full world-frame 6x6 measurement noise (assembled in aruco_R).
            const Eigen::Vector3d L = q_drone * p_body;  // lever arm, world frame
            const Eigen::Matrix<double, 6, 6> R = aruco_R(R_cam_to_world, L, R_pos, Sigma_att);

            const int32_t id = det->marker_id[i];
            auto it = aruco_tracks_.find(id);
            if (it == aruco_tracks_.end()) {
                aruco_tracks_.emplace(id, init_aruco_track(id, p_world, rvec_world, now_s, R));
            } else {
                it->second.predict(Q);
                it->second.update(z_meas, H, R, now_s);
            }
        }
    }

    // 1 Hz publish timer
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

            // Convert rvec to quaternion for the message
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

    // Helpers
    Eigen::Vector3d project_to_3d(float u, float v, const Image& depth_img) const
    {
        const int iu = static_cast<int>(std::round(u));
        const int iv = static_cast<int>(std::round(v));
        if (iu < 0 || iv < 0 ||
            iu >= static_cast<int>(depth_img.width) ||
            iv >= static_cast<int>(depth_img.height))
            return Eigen::Vector3d::Zero();

        const uint16_t d_mm = *reinterpret_cast<const uint16_t*>(depth_img.data.data() + iv * depth_img.step + iu * sizeof(uint16_t));
        if (d_mm == 0) return Eigen::Vector3d::Zero();

        const double z = d_mm / 1000.0;
        return { (u - cx_) * z / fx_, (v - cy_) * z / fy_, z };
    }

    Eigen::Vector3d to_global(const Eigen::Vector3d& p_body,
                               const PoseCovStamped&   pose) const
    {
        const auto& o = pose.pose.pose.orientation;
        const auto& t = pose.pose.pose.position;
        const Eigen::Quaterniond q(o.w, o.x, o.y, o.z);
        return q * p_body + Eigen::Vector3d(t.x, t.y, t.z);
    }

    // Types
    using ProbeSyncPolicy = message_filters::sync_policies::ApproximateTime<
        ProbeDetections, Image, PoseCovStamped>;
    using ProbeSync = message_filters::Synchronizer<ProbeSyncPolicy>;

    using ArucoSyncPolicy = message_filters::sync_policies::ApproximateTime<
        ArucoDetections, PoseCovStamped>;
    using ArucoSync = message_filters::Synchronizer<ArucoSyncPolicy>;


    // Members
    double          fx_, fy_, cx_, cy_, baseline_;
    Eigen::Matrix4d T_cam_to_drone_;
    int             min_obs_probe_;
    int             min_obs_aruco_;
    double          max_distance_;
    double          merge_distance_;

    std::vector<ProbeTrack>       probe_tracks_;
    std::map<int32_t, ArucoTrack> aruco_tracks_;
    uint32_t                      next_probe_id_{0};

    message_filters::Subscriber<ProbeDetections> probe_det_sub_;
    message_filters::Subscriber<Image>           depth_sub_;
    message_filters::Subscriber<PoseCovStamped>  probe_pose_sub_;
    std::shared_ptr<ProbeSync>                   probe_sync_;

    message_filters::Subscriber<ArucoDetections> aruco_det_sub_;
    message_filters::Subscriber<PoseCovStamped>  aruco_pose_sub_;
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
