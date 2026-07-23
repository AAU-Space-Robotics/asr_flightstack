#pragma once



struct EulerAngles { //!!! Might use double instead when ros
    float roll  = 0.0;
    float pitch = 0.0;
    float yaw   = 0.0;
};

struct DroneInformation {
    float battery_values_M[4] = {0.0, 0.0, 0.0, 0.0};
    float battery_values_C[4] = {0.0, 0.0, 0.0, 0.0};
    float motor_speed[4] = {0.0, 0.0, 0.0, 0.0};
    float xyz_pos[3] = {0.0, 0.0, 0.0};
    float tgt_pos[3] = {0.0, 0.0, 0.0};
    float velocity[3] = {0.0, 0.0, 0.0};
    EulerAngles orientation;

};