#ifndef PATHPLANNER_H
#define PATHPLANNER_H

#include <eigen3/Eigen/Dense>
#include "transformations.h"
#include <vector>
#include <mutex>

struct TrajectoryPoint {
    double position;
    double velocity;
    double acceleration;
};

struct FullTrajectoryPoint {
    Eigen::Vector3d position;        // x, y, z
    Eigen::Quaterniond orientation;  // Quaternion for yaw
    Eigen::Vector3d velocity;
    Eigen::Vector3d acceleration;
};

struct trajectorySegment {
    std::vector<double> coefficient;
};

struct Waypoint {
    Eigen::Vector3d position;
    double yaw;
    double linear_velocity;   // 0 = use default
    double angular_velocity;  // 0 = use default
};

struct TrajectorySegmentInfo {
    double start_time;
    double duration;
    int segment_index;  // Which waypoint segment this belongs to
};

struct ConstraintCheckResult {
    bool satisfied;
    double max_velocity;
    double max_acceleration;
    double time_at_max_velocity;
    double time_at_max_acceleration;
};

enum trajectoryMethod {
    MIN_SNAP
};

struct QuinticCoeffs{
    std::array<double, 6> coeffs;

    static QuinticCoeffs solve(double init_pos, double final_pos, double T,
                              double init_vel = 0, double final_vel = 0,
                              double init_acc = 0, double final_acc = 0) {
       double T2 = T * T, T3 = T2 * T, T4 = T3 * T, T5 = T4 * T;
       double A[6][6] = {
        {0, 0, 0, 0, 0, 1},
        {T5, T4, T3, T2, T, 1},
        {0, 0, 0, 0, 1, 0},
        {5*T4, 4*T3, 3*T2, 2*T, 1, 0},
        {0, 0, 0, 2, 0, 0},
        {20*T3, 12*T2, 6*T, 2, 0, 0}
       };

       double b[6] = {init_pos, final_pos, init_vel, final_vel, init_acc, final_acc};

        // Gaussian stuff
        for (int i= 0; i < 6; ++i) {
            int piv = i;
            for ( int k = i + 1; k < 6; ++k) {
                if (std::abs(A[k][i]) > std::abs(A[piv][i])) {
                    piv = k;
                }
            }
            std::swap(A[i], A[piv]);
            std::swap(b[i], b[piv]);
            for (int k = i + 1; k < 6; ++k) {
                double factor = A[k][i] / A[i][i];
                for (int j = i; j < 6; ++j) {
                    A[k][j] -= factor * A[i][j];
                }
                b[k] -= factor * b[i];
            }
        }
        QuinticCoeffs out{};
         for (int i = 5; i >= 0; --i) {
            double sum = b[i];
            for (int j = i + 1; j < 6; ++j) {
                sum -= A[i][j] * out.coeffs[j];
            }
            out.coeffs[i] = sum / A[i][i];
        }
        return out;
    }
    // Evaluate the polynomial and its derivatives at a given time t
    std::array<double, 3> eval(double x) const {
        double p   = ((((coeffs[0]*x + coeffs[1])*x + coeffs[2])*x + coeffs[3])*x + coeffs[4])*x + coeffs[5];
        double dp  = (((5*coeffs[0]*x + 4*coeffs[1])*x + 3*coeffs[2])*x + 2*coeffs[3])*x + coeffs[4];
        double ddp = ((20*coeffs[0]*x + 12*coeffs[1])*x + 6*coeffs[2])*x + 2*coeffs[3];
        return {p, dp, ddp};
    }
};

struct CircleTrajectory {
    Eigen::Vector3d position;
    Eigen::Vector3d velocity;
    Eigen::Vector3d acceleration;
    Eigen::Quaterniond orientation; // yaw = theta, i.e. facing radially outward
    double theta;      // current angle
    double theta_dot; 
};

class PathPlanner {
public:
    PathPlanner();

    float current_linear_velocity_ = 0.15;    
    float current_angular_velocity_ = 0.15;   
    float min_linear_velocity_ = 0.1;        
    float min_angular_velocity_ = 0.1;       
    float max_linear_velocity_ = 0.2;        
    float max_angular_velocity_ = 0.2;       
      
    double getTotalTime() const;

    double totalTime() const { return T; }

    // Multi-waypoint trajectory generation
    bool GenerateMultiWaypointTrajectory(
        const std::vector<Waypoint>& waypoints,
        const Eigen::Vector3d& start_velocity,
        const Eigen::Vector3d& start_acceleration,
        double start_yaw,
        trajectoryMethod method
    );
    
    // Get segment info for multi-waypoint trajectories
    std::vector<TrajectorySegmentInfo> getSegmentInfo() const;
    
    // Check if trajectory satisfies velocity/acceleration constraints
    ConstraintCheckResult checkConstraints(int num_samples = 100) const;

    bool GenerateTrajectory(
        const Eigen::Vector3d& start_pos,
        const Eigen::Vector3d& end_pos,
        const Eigen::Quaterniond& start_quat,
        const Eigen::Quaterniond& end_quat,
        const Eigen::Vector3d& vel,
        const Eigen::Vector3d& acc,
        trajectoryMethod method
    );

    bool GenerateSpinTrajectory(
        const Eigen::Vector3d& position,
        const Eigen::Quaterniond& start_quat,
        double target_yaw,
        double num_rotations,
        bool use_longest_path,
        trajectoryMethod method
    );

    TrajectoryPoint evaluatePolynomial(
        const std::vector<double>& coefficients,
        double t
    );

    FullTrajectoryPoint getTrajectoryPoint(
        double t,
        trajectoryMethod method
    );

    bool setLinearVelocity(float linear_velocity);

    bool setAngularVelocity(float angular_velocity);

    void Circle_plan(const Eigen::Vector3d& center, double radius, double revolutions, double start_theta = 0.0);

    CircleTrajectory get_Circle_Trajectory(double t) const;

    bool isCircle() const { return is_circle;}
    
private:
    double total_time;
    Eigen::Vector3d start_vel;
    Eigen::Vector3d start_acc;
    trajectorySegment segments[3]; 
    Eigen::Quaterniond start_quat;
    Eigen::Quaterniond end_quat;
    trajectorySegment yaw_segment;
    bool use_yaw_polynomial = false;

    double T = 0.0;
    bool is_circle = false;
    Eigen::Vector3d circle_center;
    QuinticCoeffs radius_poly;
    QuinticCoeffs theta_poly;
    
    // Multi-waypoint support
    std::vector<TrajectorySegmentInfo> segment_info_;
    std::vector<trajectorySegment> multi_segments_x_;
    std::vector<trajectorySegment> multi_segments_y_;
    std::vector<trajectorySegment> multi_segments_z_;
    std::vector<trajectorySegment> multi_segments_yaw_;
    std::vector<Eigen::Quaterniond> segment_start_quats_;
    std::vector<Eigen::Quaterniond> segment_end_quats_;
    bool is_multi_waypoint_ = false;

    std::vector<double> generatePolynomialCoefficients(
        double start,
        double end,
        double start_vel,
        double start_acc,
        double time,
        trajectoryMethod method
    );

    float calculateDuration(float distance, float velocity, float min_velocity, float max_velocity) const;

    Transformations transformations_;

    // Serialises trajectory generation (command thread) against trajectory evaluation
    // (100 Hz control loop). Recursive because checkConstraints() calls getTrajectoryPoint().
    mutable std::recursive_mutex planner_mutex_;
};

#endif // PATHPLANNER_H