import os
import yaml

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def _load_camera_params() -> dict:
    """Read camera intrinsics from thyra_params.yaml.

    The thyra T_cam_to_drone is designed for a NED body frame (z-down).
    The sim uses ENU identity quaternion (z-up), so that transform would
    point the camera upward.  We override it with a simple downward-facing
    transform that is consistent with the sim's coordinate convention.
    """
    path = os.path.join(
        get_package_share_directory('thyra'),
        'config', 'uav', 'thyra_params.yaml')

    with open(path) as f:
        cfg = yaml.safe_load(f)

    cam = cfg['/asr/thyra/**']['ros__parameters']['camera']

    # Thyra's T_cam_to_drone is for a NED body frame (x=North/fwd, y=East/right, z=Down).
    # The sim uses ENU body frame (x=forward, y=left, z=up).
    # Conversion: [a,b,c]_NED → [a,-b,-c]_ENU  (forward stays, right flips, down flips).
    # Result: camera optical axis = [0.707, 0, -0.707] in ENU body = 45° forward + down.
    # Drone forward = body +x; drone faces the direction of travel in the sim.
    T_sim = [0., -0.707107, 0.707107,  0.137751,
             -1.,  0.,       0.,        0.018467,
              0., -0.707107, -0.707107, -0.12126,
              0.,  0.,        0.,        1.      ]

    return {
        'fx':  cam['focal_length']['x'],
        'fy':  cam['focal_length']['y'],
        'cx':  cam['principal_point']['x'],
        'cy':  cam['principal_point']['y'],
        'img_width':  cam['image_size']['width'],
        'img_height': cam['image_size']['height'],
        'camera_to_drone_transform': T_sim,
    }


def generate_launch_description():
    camera_params = _load_camera_params()

    detection_filter = Node(
        package='asr_perception',
        executable='detection_filter',
        name='detection_filter',
        parameters=[{
            **camera_params,
            'min_observations':       3,
            'merge_distance_m':       0.6,
            'max_distance_m':         4.0,
            'aruco_min_observations': 2,
            'publish_rate_hz':        1.0,
        }],
    )

    probe_filter_sim = Node(
        package='asr_perception',
        executable='probe_filter_sim.py',
        name='probe_filter_sim',
        parameters=[{
            **camera_params,
            # Four probes at known world positions (on the ground, z=0)
            'probe_positions':    [1.0,  1.0, 0.0,
                                   -1.0, 2.5, 0.0,
                                   2.0, -1.0, 0.0,
                                   -2.0, -2.0, 0.0],
            # Lawnmower scan: 6×6 m area at 2.5 m altitude, 0.4 m step → ~450 waypoints
            'scan_altitude':       2.5,
            'scan_x_min':         -4.0,
            'scan_x_max':          4.0,
            'scan_y_min':         -3.0,
            'scan_y_max':          3.0,
            'scan_step':           0.4,
            'detection_noise_px':  5.0,
            # D435i depth: σ_z = coeff * z²  →  ~9 mm RMS at 2.5 m
            'depth_noise_coeff':   0.0015,
            # Drone pose position noise: 2 cm σ per axis → ±4 cm (~2σ) models RTK GPS
            'pose_pos_noise_m':    0.02,
            'publish_rate_hz':    10.0,
        }],
    )

    return LaunchDescription([detection_filter, probe_filter_sim])
