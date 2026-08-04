#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>


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
};

JoystickState PollJoystick(int joystick_id = GLFW_JOYSTICK_1);
ManualControlValues ComputeManualControl(const JoystickState& js, float dead_zone = 0.05f);