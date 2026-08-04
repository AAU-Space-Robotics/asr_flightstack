#include "manual.h"
#include <algorithm>
#include <cmath>

JoystickState PollJoystick(int joystick_id)
{
    JoystickState state;
    state.connected = glfwJoystickPresent(joystick_id);
    if (!state.connected) return state;

    const char* name = glfwGetJoystickName(joystick_id);
    if (name) state.name = name;

    std::cout << "DEBUG is_gamepad=" << glfwJoystickIsGamepad(joystick_id) << std::endl;

    int axis_count;
    const float* axes = glfwGetJoystickAxes(joystick_id, &axis_count);
    std::cout << "DEBUG axis_count=" << axis_count << " axes_ptr_null=" << (axes == nullptr) << std::endl;

    state.axis_count = std::min(axis_count, 16);
    for (int i = 0; i < state.axis_count; i++) state.axes[i] = axes[i];

    int button_count;
    const unsigned char* buttons = glfwGetJoystickButtons(joystick_id, &button_count);
    state.button_count = std::min(button_count, 16);
    for (int i = 0; i < state.button_count; i++) state.buttons[i] = buttons[i];

    return state;
}

static float apply_deadzone(float v, float dz) {
    return std::abs(v) > dz ? v : 0.0f;
}

ManualControlValues ComputeManualControl(const JoystickState& js, float dead_zone)
{
    ManualControlValues out;
    if (!js.connected) return out;

    out.roll         = apply_deadzone(js.axes[0], dead_zone);
    out.pitch         = apply_deadzone(-js.axes[1], dead_zone);
    out.yaw_velocity  = apply_deadzone(js.axes[2], dead_zone);
    out.thrust        = apply_deadzone(-js.axes[4], dead_zone);

    return out;
}
