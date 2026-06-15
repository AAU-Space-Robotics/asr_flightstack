std_msgs/Header header
int32 num_detections
int32[] marker_id
# tvec: 3*num_detections floats — [x0,y0,z0, x1,y1,z1, ...] in camera frame (metres)
float32[] tvec
# rvec: 3*num_detections floats — Rodrigues rotation vector per marker
float32[] rvec
