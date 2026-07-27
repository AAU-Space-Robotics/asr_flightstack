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
            'merge_distance_m':       0.5,
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
            # Lawnmower scan: 8×6 m area at 2.5 m altitude, 0.4 m step → 336 waypoints.
            # Altitude stays at 2.5 m: the camera looks 45° forward+down, so the
            # ground slant range is already ~3.5 m and max_distance_m/depth_max are
            # 4.0 — raising altitude pushes detections out of range, starving both
            # the filter and the average baseline rather than making it a fair test.
            'scan_altitude':       2.5,
            'scan_x_min':         -4.0,
            'scan_x_max':          4.0,
            'scan_y_min':         -3.0,
            'scan_y_max':          3.0,
            'scan_step':           0.4,
            'detection_noise_px':  5.0,
            # ── HARDER STATIC-PROBE REGIME ──────────────────────────────────────
            # Probes stay static (the filter's design point); we make the *sensing*
            # tougher. White noise (pixels, depth, position) averages out for both
            # the filter and a plain mean, so the real differentiator is CORRELATED
            # attitude error: a slow tilt/yaw drift the mean cannot remove but the
            # filter's bias state is built to absorb. We raise both its magnitude
            # and its correlation toward that regime.
            # D435i depth: σ_z = coeff * z²  →  ~16 mm RMS at 2.5 m (was 9 mm)
            'depth_noise_coeff':   0.0025,
            # Drone pose position noise: 2 cm σ per axis → ±4 cm (~2σ) models RTK GPS
            'pose_pos_noise_m':    0.02,
            # Attitude estimate noise, 1-σ: tilt (roll/pitch) and yaw, both raised.
            'pose_tilt_noise_deg': 1.5,   # was 0.5
            'pose_yaw_noise_deg':  3.0,   # was 1.0
            # Slow AR(1) attitude drift: 0.97 ≈ 33-frame correlation, so the error
            # does NOT average out over a probe's visible window — the mean keeps a
            # residual offset the bias-augmented filter should be able to track.
            'pose_att_correlation': 0.97, # was 0.9
            # ── DETECTOR IMPERFECTIONS — the decisive real-world test ───────────
            # A real YOLO stream drops true probes and fires on ground clutter.
            # The filter's gating + 3-observation persistence must survive this;
            # the oracle average baseline cannot see clutter (it knows truth), so
            # the real question is whether the FILTER stays at 4 clean probes or
            # lets clutter coalesce into ghost tracks (the merge can help it do so).
            'detection_miss_prob':        0.15,  # drop 15% of true detections
            'false_detections_per_frame': 0.02,  # rare clutter: ~1 per 50 frames (~7 / scan)
            'publish_rate_hz':    10.0,
        }],
    )

    return LaunchDescription([detection_filter, probe_filter_sim])
