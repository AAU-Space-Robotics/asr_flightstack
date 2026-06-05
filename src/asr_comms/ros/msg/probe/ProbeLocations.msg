std_msgs/Header header
uint32 num_probes
float32[] positions  # flattened 3×N global frame: [x0,y0,z0, x1,y1,z1, ...]
float32[] uncertainty  # flattened 3×N position std-dev from Kalman P: [σx0,σy0,σz0, ...]
