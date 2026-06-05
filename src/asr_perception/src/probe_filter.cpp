#include <rclcpp/rclcpp.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <asr_comms/msg/probe_detections.hpp>
#include <asr_comms/msg/probe_locations.hpp>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <vector>
#include <cstdint>
#include <limits>

using ProbeDetections = asr_comms::msg::ProbeDetections;
using ProbeLocations  = asr_comms::msg::ProbeLocations;
using Image           = sensor_msgs::msg::Image;
using PoseStamped     = geometry_msgs::msg::PoseStamped;

// ---------------------------------------------------------------------------
// ProbeTrack — one tracked probe
// State:       x = [px, py, pz, bx, by, bz]^T   (6x1)
// Observation: z = [px, py, pz]^T                (3x1)
// Model:       z = H*x + noise,  H = [I | I]
//              position is measured directly; bias drifts slowly via Q
// ---------------------------------------------------------------------------
struct ProbeTrack {
    uint32_t id;
    int      observations{0};
    double   last_update_s{0.0};

    Eigen::Matrix<double, 6, 1> x;   // state:               [position | bias]
    Eigen::Matrix<double, 6, 6> P;   // state covariance
    Eigen::Matrix<double, 3, 3> S;   // innovation covariance (cached for gating)

    ProbeTrack() = delete;

    ProbeTrack(uint32_t id, const Eigen::Vector3d& z_init, double now_s)
        : id(id), observations(1), last_update_s(now_s)
    {
        x.head<3>() = z_init;
        x.tail<3>() = Eigen::Vector3d::Zero();           // bias unknown, start at zero

        P = Eigen::Matrix<double, 6, 6>::Zero();
        P.topLeftCorner<3,3>()     = (0.05 * 0.05) * Eigen::Matrix3d::Identity(); // sigma_meas²
        P.bottomRightCorner<3,3>() = (0.10 * 0.10) * Eigen::Matrix3d::Identity(); // bias unknown
        
        const auto Hk = H();
        S = Hk * P * Hk.transpose() + R(); // ~(0.05² + 0.05²) * I  →  σ ≈ 7 cm, gate ≈ 20 cm
    }

    // Observation matrix:  z = [I | I] * x  →  measures position + bias
    static Eigen::Matrix<double, 3, 6> H() {
        Eigen::Matrix<double, 3, 6> mat = Eigen::Matrix<double, 3, 6>::Zero();
        mat.leftCols<3>()  = Eigen::Matrix3d::Identity();
        mat.rightCols<3>() = Eigen::Matrix3d::Identity();
        return mat;
    }

    // Process noise — position and bias drift slowly
    static Eigen::Matrix<double, 6, 6> Q() {
        constexpr double sigma_pos  = 0.01;   // meters
        constexpr double sigma_bias = 0.001;  // meters

        Eigen::Matrix<double, 6, 6> mat = Eigen::Matrix<double, 6, 6>::Zero();
        mat.topLeftCorner<3,3>()     = (sigma_pos  * sigma_pos)  * Eigen::Matrix3d::Identity();
        mat.bottomRightCorner<3,3>() = (sigma_bias * sigma_bias) * Eigen::Matrix3d::Identity();
        return mat;
    }

    // Measurement noise — scaled by detection confidence
    static Eigen::Matrix<double, 3, 3> R(double confidence = 1.0) {
        constexpr double sigma_meas = 0.05;   // meters
        return (sigma_meas * sigma_meas / confidence) * Eigen::Matrix3d::Identity();
    }

    // Mahalanobis distance from a candidate measurement — used for data association
    double mahalanobis(const Eigen::Vector3d& z) const {
        const Eigen::Vector3d innov = z - H() * x;
        // S.ldlt().solve is more stable than S.inverse(); .dot avoids 1×1 matrix.
        return std::sqrt(innov.dot(S.ldlt().solve(innov)));
    }

    // Predict — grow covariance (static model, probes don't move)
    void predict() {
        P = P + Q();
    }

    // Update with a new 3-D measurement
    void update(const Eigen::Vector3d& z_meas, double meas_confidence, double now_s) {
        const auto Hk = H();
        const auto Rk = R(meas_confidence);

        S = Hk * P * Hk.transpose() + Rk;
        // Explicit type forces immediate evaluation — avoids dangling reference to
        // the temporary LLT object that auto would otherwise capture lazily.
        const Eigen::Matrix<double, 6, 3> K = S.llt().solve(Hk * P).transpose();
        const Eigen::Matrix<double, 6, 6> IKH =
            Eigen::Matrix<double,6,6>::Identity() - K * Hk;

        x = x + K * (z_meas - Hk * x);
        P = IKH * P * IKH.transpose() + K * Rk * K.transpose();  // Joseph form

        ++observations;
        last_update_s = now_s;
    }

    // Position estimate
    Eigen::Vector3d position() const { return x.head<3>(); }

    // Bias estimate
    Eigen::Vector3d bias() const { return x.tail<3>(); }

    // Per-axis 1-sigma uncertainty from the filter — this is your confidence
    Eigen::Vector3d position_std() const {
        return P.topLeftCorner<3,3>().diagonal().cwiseSqrt();
    }
};

// ---------------------------------------------------------------------------
// ProbeFilter node
// ---------------------------------------------------------------------------
class ProbeFilter : public rclcpp::Node {
public:
    ProbeFilter() : Node("probe_filter")
    {
        // --- Parameters ---
        declare_parameter("fx", 425.88);
        declare_parameter("fy", 425.88);
        declare_parameter("cx", 430.51);
        declare_parameter("cy", 238.53);
        declare_parameter("camera_to_drone_transform", std::vector<double>(16, 0.0));
        declare_parameter("min_observations",   2);
        declare_parameter("merge_distance_m",   0.6);
        declare_parameter("max_distance_m",     4.0);

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
            RCLCPP_WARN(get_logger(), "camera_to_drone_transform param missing/wrong size — using identity");
            T_cam_to_drone_ = Eigen::Matrix4d::Identity();
        }

        min_observations_ = get_parameter("min_observations").as_int();
        max_distance_     = get_parameter("max_distance_m").as_double();

        // --- Sync subscribers ---
        auto qos = rclcpp::QoS(5).best_effort();

        det_sub_.subscribe(this,   "probe_detector/detections",    qos.get_rmw_qos_profile());
        depth_sub_.subscribe(this, "out/cam/synced/depth",          qos.get_rmw_qos_profile());
        pose_sub_.subscribe(this,  "out/cam/synced/pose",           qos.get_rmw_qos_profile());

        sync_ = std::make_shared<Synchronizer>(
            SyncPolicy(10), det_sub_, depth_sub_, pose_sub_);
        sync_->setMaxIntervalDuration(rclcpp::Duration::from_seconds(0.15));
        sync_->registerCallback(
            std::bind(&ProbeFilter::on_data, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));

        probe_pub_ = create_publisher<ProbeLocations>("probe_detector/locations", 10);

        RCLCPP_INFO(get_logger(), "Probe filter ready");
    }

private:
    // --- Sync callback ---
    void on_data(
        const ProbeDetections::ConstSharedPtr& detections,
        const Image::ConstSharedPtr&           depth,
        const PoseStamped::ConstSharedPtr&     pose)
    {
        const double now_s = rclcpp::Time(detections->header.stamp).seconds();

        std::vector<Eigen::Vector3d> measurements;
        std::vector<float>           confidences;

        for (uint32_t i = 0; i < detections->num_detections; ++i) {
            const float cx_px = detections->centroid_x[i];
            const float cy_px = detections->centroid_y[i];

            // 1. Project pixel + depth → 3-D in camera frame
            const Eigen::Vector3d p_cam = project_to_3d(cx_px, cy_px, *depth);
            if (p_cam.z() <= 0.0 || p_cam.norm() > max_distance_) continue;

            // 2. Transform camera frame → drone body frame
            const Eigen::Vector4d p_h(p_cam.x(), p_cam.y(), p_cam.z(), 1.0);
            const Eigen::Vector3d p_body = (T_cam_to_drone_ * p_h).head<3>();

            // 3. Transform drone body frame → global frame
            const Eigen::Vector3d p_global = to_global(p_body, *pose);

            measurements.push_back(p_global);
            confidences.push_back(detections->confidence[i]);
        }

        update_tracks(measurements, confidences, now_s);
        publish_locations(detections->header);
    }

    // --- 3-D projection ---
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
        const double x = (u - cx_) * z / fx_;
        const double y = (v - cy_) * z / fy_;
        return {x, y, z};
    }

    // --- Global frame transform ---
    Eigen::Vector3d to_global(const Eigen::Vector3d& p_body,
                               const PoseStamped&      pose) const
    {
        const auto& o = pose.pose.orientation;
        const auto& t = pose.pose.position;
        const Eigen::Quaterniond q(o.w, o.x, o.y, o.z);
        return q * p_body + Eigen::Vector3d(t.x, t.y, t.z);
    }

    // --- Track management ---
    void update_tracks(const std::vector<Eigen::Vector3d>& measurements,
                       const std::vector<float>&            confidences,
                       double                               now_s)
    {
        // sqrt(chi2(3, 0.95)) — 95% confidence gate in 3D
        constexpr double CHI2_GATE = 2.79;

        for (size_t i = 0; i < measurements.size(); ++i) {
            // Seed the very first track unconditionally
            if (tracks_.empty()) {
                tracks_.push_back(ProbeTrack(next_id_++, measurements[i], now_s));
                continue;
            }

            // Find nearest track in Mahalanobis space
            double      best_dist  = std::numeric_limits<double>::max();
            ProbeTrack* best_track = nullptr;

            for (auto& track : tracks_) {
                const double d = track.mahalanobis(measurements[i]);
                if (d < best_dist) {
                    best_dist  = d;
                    best_track = &track;
                }
            }

            if (best_dist < CHI2_GATE) {
                // Statistically consistent — update existing track
                best_track->predict();
                best_track->update(measurements[i], confidences[i], now_s);
            } else {
                // Outside all gates — new probe
                tracks_.push_back(ProbeTrack(next_id_++, measurements[i], now_s));
            }
        }
    }

    // --- Publish ---
    void publish_locations(const std_msgs::msg::Header& header)
    {
        ProbeLocations msg;
        msg.header = header;

        for (const auto& track : tracks_) {
            if (track.observations < min_observations_) continue;

            const auto pos = track.position();
            const auto std = track.position_std();

            msg.positions.push_back(static_cast<float>(pos.x()));
            msg.positions.push_back(static_cast<float>(pos.y()));
            msg.positions.push_back(static_cast<float>(pos.z()));

            msg.uncertainty.push_back(static_cast<float>(std.x()));
            msg.uncertainty.push_back(static_cast<float>(std.y()));
            msg.uncertainty.push_back(static_cast<float>(std.z()));

            ++msg.num_probes;
        }

        probe_pub_->publish(msg);
    }

    // --- Types ---
    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
        ProbeDetections, Image, PoseStamped>;
    using Synchronizer = message_filters::Synchronizer<SyncPolicy>;

    // --- Members ---
    double fx_, fy_, cx_, cy_;
    Eigen::Matrix4d T_cam_to_drone_;
    int    min_observations_;
    double max_distance_;

    std::vector<ProbeTrack> tracks_;
    uint32_t                next_id_{0};

    message_filters::Subscriber<ProbeDetections> det_sub_;
    message_filters::Subscriber<Image>           depth_sub_;
    message_filters::Subscriber<PoseStamped>     pose_sub_;
    std::shared_ptr<Synchronizer>                sync_;

    rclcpp::Publisher<ProbeLocations>::SharedPtr probe_pub_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ProbeFilter>());
    rclcpp::shutdown();
    return 0;
}