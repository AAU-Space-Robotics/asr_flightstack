#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <cstring>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cerrno>

struct JoystickState {
    bool connected = false;
    std::string name;
    float axes[16] = {0};
    unsigned char buttons[16] = {0};
    int axis_count = 0;
    int button_count = 0;
};

struct ManualControlValues {
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw_velocity = 0.0f;
    float thrust = 0.0f;
    bool manual_engage = false;  // deadman held
    bool arm_pressed = false;
    bool estop_pressed = false;
    bool land_pressed = false;
};

struct ManualController{
    bool InitJoystick(const char* device_path = "/dev/input/js0");
    JoystickState PollJoystick();
    void CloseJoystick();
    ManualControlValues ComputeManualControl(const JoystickState& js, float dead_zone = 0.05f);
};

inline constexpr std::chrono::milliseconds RECONNECT_INTERVAL(1000);