std_msgs/Header header
int32 num_markers
int32[]   marker_id
# position:    3*num_markers floats [x0,y0,z0, x1,y1,z1, ...] world frame (metres)
float32[] position
# orientation: 4*num_markers floats [qw0,qx0,qy0,qz0, ...] world frame
float32[] orientation
# uncertainty: 3*num_markers floats position 1-sigma per axis (metres)
float32[] uncertainty
