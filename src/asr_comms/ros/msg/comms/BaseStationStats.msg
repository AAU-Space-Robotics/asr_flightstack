# Connection status
bool  basestation_connected      # true if the base station is connected

# RTK survey state
uint8 RTK_STATUS_INACTIVE  = 0
uint8 RTK_STATUS_SURVEY_IN = 1
uint8 RTK_STATUS_STREAMING = 2
uint8   rtk_status                # see RTK_STATUS_* above
float32 rtk_accuracy               # current survey-in accuracy [m]
float32 rtk_accuracy_target        # survey-in accuracy limit [m] (from survey_accuracy_mm param)
float32 rtk_survey_duration        # elapsed survey-in duration [s]
float32 rtk_survey_duration_max    # survey-in minimum duration target [s] (from survey_min_duration param)