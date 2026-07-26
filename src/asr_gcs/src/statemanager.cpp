//#include "statemanager.h"
//
//void StateManager::setAttitude(const StampedQuaternion& new_data) {
//    std::lock_guard<std::mutex> lock(attitude_data_mutex_);
//    attitude_ = new_data;   
//}
//
//StampedQuaternion StateManager::getAttitude() {
//    std::lock_guard<std::mutex> lock(attitude_data_mutex_);
//    return attitude_;   
//}