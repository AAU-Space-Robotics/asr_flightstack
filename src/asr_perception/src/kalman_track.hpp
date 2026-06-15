#pragma once
#include <Eigen/Dense>
#include <cstdint>
#include <cmath>

// Generic Kalman filter track: N-dimensional state, M-dimensional measurement.
// Caller supplies H, Q, R matrices; this struct owns state, covariance, and
// the cached innovation covariance S (used for Mahalanobis gating).
template<int N, int M>
struct KalmanTrack {
    using StateVec = Eigen::Matrix<double, N, 1>;
    using StateMat = Eigen::Matrix<double, N, N>;
    using MeasVec  = Eigen::Matrix<double, M, 1>;
    using MeasMat  = Eigen::Matrix<double, M, M>;
    using ObsMat   = Eigen::Matrix<double, M, N>;

    uint32_t id{0};
    int      observations{0};
    double   last_update_s{0.0};

    StateVec x{StateVec::Zero()};
    StateMat P{StateMat::Zero()};
    MeasMat  S{MeasMat::Identity()};  // cached innovation covariance

    // Predict step: grow covariance by Q (static / near-static target model)
    void predict(const StateMat& Q) { P = P + Q; }

    // Update step with measurement z; refreshes S, state, and covariance
    void update(const MeasVec& z, const ObsMat& H, const MeasMat& R, double now_s) {
        S = H * P * H.transpose() + R;
        // K = P H^T S^{-1}  computed via Cholesky on S
        const Eigen::Matrix<double, N, M> K = S.llt().solve(H * P).transpose();
        const StateMat IKH = StateMat::Identity() - K * H;
        x = x + K * (z - H * x);
        P = IKH * P * IKH.transpose() + K * R * K.transpose();  // Joseph form
        ++observations;
        last_update_s = now_s;
    }

    // Mahalanobis distance from candidate measurement to predicted observation
    double mahalanobis(const MeasVec& z, const ObsMat& H) const {
        const MeasVec innov = z - H * x;
        return std::sqrt(innov.dot(S.ldlt().solve(innov)));
    }

    // Convenience accessors — position is always the first 3 state elements
    Eigen::Vector3d position() const {
        return x.template head<3>();
    }
    Eigen::Vector3d position_std() const {
        return P.template topLeftCorner<3, 3>().diagonal().cwiseSqrt();
    }
};

// Probe track: [px, py, pz, bx, by, bz]  — 6-state, 3-measurement
using ProbeTrack = KalmanTrack<6, 3>;

// ArUco track: [px, py, pz, bx, by, bz, rx, ry, rz]  — 9-state, 6-measurement
// rx/ry/rz is a Rodrigues rotation vector in world frame
using ArucoTrack = KalmanTrack<9, 6>;
