#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H

#include <mutex>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Geometry>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include "state_manager.h"

// Euler angles in radians, with named fields (not an indexed vector) so callers cannot
// accidentally swap roll/yaw — the historical source of attitude-command bugs.
struct EulerAngles {
    double roll  = 0.0;
    double pitch = 0.0;
    double yaw   = 0.0;
};

class Transformations {
public:
    Transformations(){}

    // GPS Origin Management
    void setGPSOrigin(const rclcpp::Time& timestamp, double latitude, double longitude, double altitude);
    bool isGPSOriginSet() const;
    Stamped3DVector getGPSOrigin() const;

    // Coordinate Transformations
    Stamped3DVector convertGPSToGlobalPosition(const rclcpp::Time& timestamp, double latitude, double longitude, double altitude) const;
    Stamped3DVector accelerationLocalToGlobal(const rclcpp::Time& timestamp, 
                                             const Eigen::Quaterniond& attitude, 
                                             const Eigen::Vector3d& acceleration_local) const;

    // Attitude Conversions
    Eigen::Quaterniond eulerToQuaternion(double roll, double pitch, double yaw) const;
    EulerAngles quaternionToEuler(const Eigen::Quaterniond& q) const;

    // Geodetic Transformations
    Eigen::Vector3d errorGlobalToLocal(const Eigen::Vector3d& error_ned_earth, 
                                      const Eigen::Quaterniond& attitude_frd_to_ned) const;

    // Utility Functions
    double degToRad(double degrees) const;
    double radToDeg(double radians) const;
    double unwrapAngle(double angle, double max = M_PI, double min = -M_PI) const;

private:

};

#endif // TRANSFORMATIONS_H