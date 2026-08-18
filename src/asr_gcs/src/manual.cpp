#include "manual.h"
#include <algorithm>
#include <cmath>

static int joystick_fd = -1;
static JoystickState controller_state; 
static std::string g_device_path = "/dev/input/js0"; 
static std::chrono::steady_clock::time_point controller_last_reconnect_attempt;

static void TryOpenDevice()
{
    joystick_fd = open(g_device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (joystick_fd < 0) {
        controller_state.connected = false;
        return;
    }

    char name[128] = "Unknown";
    ioctl(joystick_fd, JSIOCGNAME(sizeof(name)), name);
    controller_state.name = name;
    controller_state.connected = true;

    // JSIOCGAXES / JSIOCGBUTTONS tell us how many axes/buttons this device has
    unsigned char num_axes = 0, num_buttons = 0;
    ioctl(joystick_fd, JSIOCGAXES, &num_axes);
    ioctl(joystick_fd, JSIOCGBUTTONS, &num_buttons);
    controller_state.axis_count = std::min((int)num_axes, 16);
    controller_state.button_count = std::min((int)num_buttons, 16);


    memset(controller_state.axes, 0, sizeof(controller_state.axes));
    memset(controller_state.buttons, 0, sizeof(controller_state.buttons));

    std::cout << "Joystick connected: " << controller_state.name
              << " | axes=" << controller_state.axis_count
              << " buttons=" << controller_state.button_count << std::endl;
}

bool ManualController::InitJoystick(const char* device_path)
{
    
    g_device_path = device_path;
    TryOpenDevice();
    controller_last_reconnect_attempt = std::chrono::steady_clock::now();
    return controller_state.connected;
}

JoystickState ManualController::PollJoystick()
{
    if(joystick_fd < 0) {

        auto now = std::chrono::steady_clock::now();
        if (now - controller_last_reconnect_attempt >= RECONNECT_INTERVAL) {
            TryOpenDevice();
            controller_last_reconnect_attempt = now;
        }
        return controller_state;
    }
    js_event event;
    ssize_t bytes_read;
    bool disconnected = false;
    while (read(joystick_fd, &event, sizeof(event)) == sizeof(event)){
        unsigned char type = event.type & ~JS_EVENT_INIT;

        if (type == JS_EVENT_AXIS && event.number < 16){
            controller_state.axes[event.number] = event.value / 32767.0f;
        } else if (type == JS_EVENT_BUTTON && event.number < 16){
            controller_state.buttons[event.number] = event.value;
        }
    }
    if (bytes_read < 0 && errno != EAGAIN) {
        std::cout << "Joystick disconnected." << std::endl;
        disconnected = true;
    }
    if (disconnected) {
        close(joystick_fd);
        joystick_fd = -1;
        controller_state.connected = false;
        controller_last_reconnect_attempt = std::chrono::steady_clock::now();
    }
    return controller_state;
}

static float apply_deadzone(float v, float dz) {
    return std::abs(v) > dz ? v : 0.0f;
}

void ManualController::CloseJoystick()
{
    if (joystick_fd >= 0){
        close(joystick_fd);
        joystick_fd = -1;
    }
    controller_state.connected = false;
}

ManualControlValues ManualController::ComputeManualControl(const JoystickState& js, float dead_zone)
{
    ManualControlValues out;
    if (!js.connected) return out;

    if (js.name == "8BitDo Ultimate 2 Wireless") {
        out.manual_engage = (js.buttons[2] != 0) || (js.buttons[5] != 0);
        out.arm_pressed   = js.buttons[0] != 0;
        out.estop_pressed = js.buttons[1] != 0;
        out.land_pressed  = js.buttons[3] != 0;

        if (!out.manual_engage) {
            return out;
        }

        auto trigger_pressed = [](float raw) { return (raw + 1.0f) / 2.0f; };
        const float yaw_raw = trigger_pressed(js.axes[4]) - trigger_pressed(js.axes[5]); // RT - LT

        out.roll          = apply_deadzone(js.axes[0], dead_zone);
        out.pitch         = apply_deadzone(-js.axes[1], dead_zone);
        out.thrust        = apply_deadzone(-js.axes[3], dead_zone);
        out.yaw_velocity  = apply_deadzone(yaw_raw, dead_zone);
        return out;
    }

    // Søren please fix name for other controller :)
    out.roll         = apply_deadzone(js.axes[0], dead_zone);
    out.pitch         = apply_deadzone(-js.axes[1], dead_zone);
    out.yaw_velocity  = apply_deadzone(js.axes[2], dead_zone);
    out.thrust        = apply_deadzone(-js.axes[4], dead_zone);

    return out;
}
