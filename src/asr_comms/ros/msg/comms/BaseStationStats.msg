# Connection status
bool    basestation_connected   # true if any MAVLink received within 2 s
int8    rtk_status               # 0 for inactive, 1 for survey-in and 2 for rtk streaming


# RTK survey state
float32 rtk_accuracy             # rtk_accuracy in meters
float32 rtk_accuracy_target      # rtk_accuracy target in meters
float64 rtk_survery_duration
float64 rtk_survery_duration_max


