#include "state_manager.h"
#include "transformations.h"

// Setters & Getters
void StateManager::setGlobalPosition(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(position_global_mutex_);
    position_global_ = new_data;   
}

Stamped3DVector StateManager::getGlobalPosition() {
    std::lock_guard<std::mutex> lock(position_global_mutex_);
    return position_global_;   
}

void StateManager::setGPSPosition(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(position_gps_mutex_);
    position_gps_ = new_data;   
}

Stamped3DVector StateManager::getGPSPosition() {
    std::lock_guard<std::mutex> lock(position_gps_mutex_);
    return position_gps_;   
}


// ---

void StateManager::setOrigin(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(origin_mutex_);
    origin_ = new_data;   
}

Stamped3DVector StateManager::getOrigin() {
    std::lock_guard<std::mutex> lock(origin_mutex_);
    return origin_;   
}

void StateManager::setOriginGPS(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(origin_gps_mutex_);
    origin_gps_ = new_data;   
}

Stamped3DVector StateManager::getOriginGPS() {
    std::lock_guard<std::mutex> lock(origin_gps_mutex_);
    return origin_gps_;   
}

// ---

void StateManager::setGlobalVelocity(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(velocity_global_mutex_);
    velocity_global_ = new_data;
}

Stamped3DVector StateManager::getGlobalVelocity() {
    std::lock_guard<std::mutex> lock(velocity_global_mutex_);
    return velocity_global_;  
}

// ---

void StateManager::setGlobalAcceleration(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(acceleration_global_mutex_);
    acceleration_global_ = new_data;   
}

Stamped3DVector StateManager::getGlobalAcceleration() {
    std::lock_guard<std::mutex> lock(acceleration_global_mutex_);
    return acceleration_global_;   
}

// ---

void StateManager::setAttitude(const StampedQuaternion& new_data) {
    std::lock_guard<std::mutex> lock(attitude_data_mutex_);
    attitude_ = new_data;   
}

StampedQuaternion StateManager::getAttitude() {
    std::lock_guard<std::mutex> lock(attitude_data_mutex_);
    return attitude_;   
}

// ---

void StateManager::setTargetPositionProfile(const Stamped4DVector& new_data) {
    std::lock_guard<std::mutex> lock(target_position_profile_mutex_);
    target_position_profile_ = new_data;   
}

Stamped4DVector StateManager::getTargetPositionProfile() {
    std::lock_guard<std::mutex> lock(target_position_profile_mutex_);
    return target_position_profile_;   
}

void StateManager::setTargetVelocityProfile(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(target_velocity_profile_mutex_);
    target_velocity_profile_ = new_data;   
}

Stamped3DVector StateManager::getTargetVelocityProfile() {
    std::lock_guard<std::mutex> lock(target_velocity_profile_mutex_);
    return target_velocity_profile_;
}

void StateManager::setTargetAttitude(const StampedQuaternion& new_data) {
    std::lock_guard<std::mutex> lock(target_attitude_mutex_);
    target_attitude_ = new_data;   
}

StampedQuaternion StateManager::getTargetAttitude() {
    std::lock_guard<std::mutex> lock(target_attitude_mutex_);
    return target_attitude_;   
}

// ---

void StateManager::setHeartbeat(const GCSHeartbeat& new_data) {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    gcs_heartbeat_ = new_data;   
}

GCSHeartbeat StateManager::getHeartbeat() {
    std::lock_guard<std::mutex> lock(heartbeat_mutex_);
    return gcs_heartbeat_;   
}

// ---

void StateManager::setUAVCmdAck(const UAVCmdAck& new_data) {
    std::lock_guard<std::mutex> lock(uav_cmd_ack_mutex_);
    uav_cmd_ack_ = new_data;   
}

UAVCmdAck StateManager::getUAVCmdAck() {
    std::lock_guard<std::mutex> lock(uav_cmd_ack_mutex_);
    return uav_cmd_ack_;   
}

// ---

void StateManager::setUAVState(const UAVState& new_data) {
    std::lock_guard<std::mutex> lock(uav_state_mutex_);
    uav_state_ = new_data;   
}

UAVState StateManager::getUAVState() {
    std::lock_guard<std::mutex> lock(uav_state_mutex_);
    return uav_state_;   
}

// ---

void StateManager::setBatteryState(const BatteryState& new_data, size_t battery_index) {
    std::lock_guard<std::mutex> lock(battery_state_mutex_);
    battery_states_[battery_index < kNumBatteries ? battery_index : 0] = new_data;
}

BatteryState StateManager::getBatteryState(size_t battery_index) {
    std::lock_guard<std::mutex> lock(battery_state_mutex_);
    return battery_states_[battery_index < kNumBatteries ? battery_index : 0];
}

// ---

void StateManager::setManualControlInput(const Stamped4DVector& new_data) {
    std::lock_guard<std::mutex> lock(manual_control_input_mutex_);
    manual_control_input_ = new_data;   
}

Stamped4DVector StateManager::getManualControlInput() {
    std::lock_guard<std::mutex> lock(manual_control_input_mutex_);
    return manual_control_input_; 
}

// ---

void StateManager::setAccelerationError(const AccelerationError& new_data) {
    std::lock_guard<std::mutex> lock(acceleration_error_mutex_);
    acceleration_error_ = new_data;   
}

AccelerationError StateManager::getAccelerationError() {
    std::lock_guard<std::mutex> lock(acceleration_error_mutex_);
    return acceleration_error_;   
}

// ---

void StateManager::setPositionError(const PositionError& new_data) {
    std::lock_guard<std::mutex> lock(position_error_mutex_);
    position_error_ = new_data;   
}

PositionError StateManager::getPositionError() {
    std::lock_guard<std::mutex> lock(position_error_mutex_);
    return position_error_;   
}

void StateManager::setGPSState(const GPSState& new_data) {
    std::lock_guard<std::mutex> lock(gps_state_mutex_);
    gps_state_ = new_data;   
}
GPSState StateManager::getGPSState() {
    std::lock_guard<std::mutex> lock(gps_state_mutex_);
    return gps_state_;   
}

// ---

void StateManager::setLatestControlSignalPositionOnly(const Eigen::Vector4d& new_data) {
    std::lock_guard<std::mutex> lock(latest_control_signal_position_only_mutex_);
    latest_control_signal_position_only_ = new_data;   
}
Eigen::Vector4d StateManager::getLatestControlSignalPositionOnly() {
    std::lock_guard<std::mutex> lock(latest_control_signal_position_only_mutex_);
    return latest_control_signal_position_only_;   
}

void StateManager::setLatestControlSignalPosition(const Eigen::Vector3d& new_data) {
    std::lock_guard<std::mutex> lock(latest_control_signal_position_mutex_);
    latest_control_signal_position_ = new_data;   
}
Eigen::Vector3d StateManager::getLatestControlSignalPosition() {
    std::lock_guard<std::mutex> lock(latest_control_signal_position_mutex_);
    return latest_control_signal_position_;   
}

void StateManager::setLatestControlSignalVelocity(const Eigen::Vector4d& new_data) {
    std::lock_guard<std::mutex> lock(latest_control_signal_velocity_mutex_);
    latest_control_signal_velocity_ = new_data;   
}
Eigen::Vector4d StateManager::getLatestControlSignalVelocity() {
    std::lock_guard<std::mutex> lock(latest_control_signal_velocity_mutex_);
    return latest_control_signal_velocity_;   
}

// ---


void StateManager::setGroundDistanceState(const Stamped3DVector& new_data) {
    std::lock_guard<std::mutex> lock(ground_distance_state_mutex_);
    ground_distance_state_ = new_data;   
}
Stamped3DVector StateManager::getGroundDistanceState() {
    std::lock_guard<std::mutex> lock(ground_distance_state_mutex_);
    return ground_distance_state_;   
}

// ---

void StateManager::setActuatorSpeeds(const Stamped4DVector& new_data) {
    std::lock_guard<std::mutex> lock(actuator_speeds_mutex_);
    actuator_speeds_ = new_data;   
}
Stamped4DVector StateManager::getActuatorSpeeds() {
    std::lock_guard<std::mutex> lock(actuator_speeds_mutex_);
    return actuator_speeds_;
}

// -- 
void StateManager::setGlobalProbeLocations(const GlobalProbeLocations& new_data) {
    std::lock_guard<std::mutex> lock(probe_global_locations_mutex_);
    probe_global_locations_ = new_data;
}

GlobalProbeLocations StateManager::getGlobalProbeLocations() {
    std::lock_guard<std::mutex> lock(probe_global_locations_mutex_);
    return probe_global_locations_;
}

void StateManager::setGimbalPriority(const bool priority) {
    std::lock_guard<std::mutex> lock(gimbal_priority_mutex_);
    gimbal_priority_ = priority;
}

bool StateManager::getGimbalPriority() {
    std::lock_guard<std::mutex> lock(gimbal_priority_mutex_);
    return gimbal_priority_;
}

void StateManager::setRTKstatus(const BaseStateInfo& new_data){
    std::lock_guard<std::mutex> lock(rtk_data_mutex_);
    rtk_data_ = new_data;
}

BaseStateInfo StateManager::getRTKstatus(){
    std::lock_guard<std::mutex> lock(rtk_data_mutex_);
    return rtk_data_;
}

// Get bundled trajectory initialization state
TrajectoryInitState StateManager::getTrajectoryInitState() {
    // Lock all necessary mutexes
    std::lock_guard<std::mutex> att_lock(attitude_data_mutex_);
    std::lock_guard<std::mutex> vel_lock(velocity_global_mutex_);
    std::lock_guard<std::mutex> acc_lock(acceleration_global_mutex_);
    std::lock_guard<std::mutex> profile_lock(target_position_profile_mutex_);
    std::lock_guard<std::mutex> position_lock(position_global_mutex_);
    
    Eigen::Quaterniond quat = attitude_.data.normalized();
    Transformations transformations;
    EulerAngles euler = transformations.quaternionToEuler(quat);

    return {
        .position = {position_global_.x(), position_global_.y(), position_global_.z()},
        .position_target_prev = {target_position_profile_.x(), target_position_profile_.y(), target_position_profile_.z()},
        .orientation = quat,
        .velocity = velocity_global_.data,
        .acceleration = acceleration_global_.data,
        .yaw = euler.yaw
    };
}

void StateManager::setArminState(const bool state){
    std::lock_guard<std::mutex> lock(arming_state_mutex_);
    arming_state = state;
}

bool StateManager::getArminState(){
    std::lock_guard<std::mutex> lock(arming_state_mutex_);
    return arming_state;
}

// End of Setters & Getters

