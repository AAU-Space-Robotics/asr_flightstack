#!/usr/bin/env python3
"""
probe_filter_sim — synthetic test node for the detection_filter (probe + ArUco).

Bypasses the YOLO / ArUco detectors. Back-projects known ground-truth probe and
marker poses into the camera frame to produce synthetic ProbeDetections and
ArucoDetections, and renders a realistic flat-ground (z_world = 0) depth image,
so the filter's full 3-D-lifting + 6-DoF pipeline is exercised end-to-end.

The drone follows a lawnmower scan pattern at fixed altitude. The published pose
carries realistic estimator noise (RTK-level position + EKF-like tilt/yaw drift),
so the measurement covariance R — including the attitude/lever-arm and ArUco
orientation terms — is tested against the noise it claims to model.

Topics published (all with matching stamps for ApproximateTime sync):
  probe_detector/detections       ProbeDetections  — synthetic pixel centroids
  aruco_detector/detections       ArucoDetections  — synthetic tvec/rvec per marker
  out/cam/synced/depth            sensor_msgs/Image (16UC1, mm) — flat ground
  out/cam/synced/pose             geometry_msgs/PoseWithCovarianceStamped — pose + cov
  probe_filter_sim/ground_truth   ProbeLocations   — GT positions for plotting

Subscribes:
  probe_detector/locations        ProbeLocations  — probe filter output; error vs GT
  aruco_detector/locations        ArucoLocations  — marker filter output; error vs GT

A live matplotlib window shows the top-down XY view:
  ★ gold stars     — ground-truth probe positions
  ■ purple squares — ground-truth ArUco markers (tick = yaw)
  ▲ blue arrow     — current drone position (fading trail)
  ▭ blue dashed    — camera ground footprint (range-clipped field of view)
  ● red dots       — noisy probe measurements (depth + pixel + pose error)
  ✕ black crosses  — clutter (false) detections fed to the filter
  ● green dots     — probe filter estimates; ring = 1-σ uncertainty
  ◆ orange diam.   — naive running mean of probe measurements (baseline)
  ■ magenta sq.    — ArUco filter estimates (tick = yaw); ring = 1-σ

Default camera_to_drone_transform assumes the camera looks straight down
(camera z-axis → world −z) with drone body aligned to the world ENU frame.
  T = [[1,0,0,0],[0,-1,0,0],[0,0,-1,0],[0,0,0,1]]  (row-major, flattened)
Pass the same value to probe_filter to keep the coordinate frames consistent.
"""

import threading
from collections import deque

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSReliabilityPolicy

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

from geometry_msgs.msg import PoseWithCovarianceStamped
from sensor_msgs.msg import Image
from asr_comms.msg import (ProbeDetections, ProbeLocations,
                           ArucoDetections, ArucoLocations)


def _quat_to_rot(qw: float, qx: float, qy: float, qz: float) -> np.ndarray:
    """Unit quaternion → 3×3 rotation matrix (world_R_body)."""
    n = np.sqrt(qw**2 + qx**2 + qy**2 + qz**2)
    qw, qx, qy, qz = qw/n, qx/n, qy/n, qz/n
    return np.array([
        [1-2*(qy**2+qz**2), 2*(qx*qy-qz*qw),   2*(qx*qz+qy*qw)],
        [2*(qx*qy+qz*qw),   1-2*(qx**2+qz**2), 2*(qy*qz-qx*qw)],
        [2*(qx*qz-qy*qw),   2*(qy*qz+qx*qw),   1-2*(qx**2+qy**2)],
    ], dtype=np.float64)


def _rvec_to_rot(rvec) -> np.ndarray:
    """Rodrigues rotation vector → 3×3 rotation matrix."""
    rvec = np.asarray(rvec, dtype=np.float64)
    theta = float(np.linalg.norm(rvec))
    if theta < 1e-9:
        return np.eye(3)
    k = rvec / theta
    K = np.array([[0.0, -k[2], k[1]],
                  [k[2], 0.0, -k[0]],
                  [-k[1], k[0], 0.0]])
    return np.eye(3) + np.sin(theta) * K + (1.0 - np.cos(theta)) * (K @ K)


def _rot_to_rvec(R: np.ndarray) -> np.ndarray:
    """3×3 rotation matrix → Rodrigues rotation vector (small/normal angles)."""
    cos_t = np.clip((np.trace(R) - 1.0) * 0.5, -1.0, 1.0)
    theta = float(np.arccos(cos_t))
    if theta < 1e-9:
        return np.zeros(3)
    v = np.array([R[2, 1] - R[1, 2],
                  R[0, 2] - R[2, 0],
                  R[1, 0] - R[0, 1]])
    n = float(np.linalg.norm(v))
    if n < 1e-9:
        return np.zeros(3)        # θ≈π (does not arise in this sim)
    return (theta / n) * v


class ProbeFilterSim(Node):
    def __init__(self):
        super().__init__('probe_filter_sim')

        # ── camera intrinsics (must match probe_filter params) ──────────────
        self.declare_parameter('fx', 425.88)
        self.declare_parameter('fy', 425.88)
        self.declare_parameter('cx', 430.51)
        self.declare_parameter('cy', 238.53)
        self.declare_parameter('img_width',  861)
        self.declare_parameter('img_height', 480)

        # Default: 45° forward-tilted camera, drone body +x = forward (ENU).
        # Converted from thyra NED transform: [a,b,c]_NED → [a,-b,-c]_ENU
        _T_default = [0., -0.707107, 0.707107,  0.137751,
                      -1.,  0.,       0.,        0.018467,
                       0., -0.707107, -0.707107, -0.12126,
                       0.,  0.,        0.,        1.      ]
        self.declare_parameter('camera_to_drone_transform', _T_default)

        self._fx  = self.get_parameter('fx').value
        self._fy  = self.get_parameter('fy').value
        self._cx  = self.get_parameter('cx').value
        self._cy  = self.get_parameter('cy').value
        self._W   = self.get_parameter('img_width').value
        self._H   = self.get_parameter('img_height').value

        T_flat = self.get_parameter('camera_to_drone_transform').value
        T = np.array(T_flat, dtype=np.float64).reshape(4, 4)
        self._R_cd = T[:3, :3]   # rotation: camera → drone body
        self._t_cd = T[:3,  3]   # camera origin in drone body frame

        # ── ground-truth probes [x0,y0,z0, x1,y1,z1, ...] ──────────────────
        self.declare_parameter('probe_positions', [1.0, 1.0, 0.0,  -1.0, 2.5, 0.0])
        pos_flat = self.get_parameter('probe_positions').value
        self._gt = np.array(pos_flat, dtype=np.float64).reshape(-1, 3)

        # ── lawnmower scan ───────────────────────────────────────────────────
        self.declare_parameter('scan_altitude',  2.5)
        self.declare_parameter('scan_x_min',    -3.0)
        self.declare_parameter('scan_x_max',     3.0)
        self.declare_parameter('scan_y_min',    -3.0)
        self.declare_parameter('scan_y_max',     3.0)
        self.declare_parameter('scan_step',      0.4)
        self.declare_parameter('detection_noise_px', 1.5)
        self.declare_parameter('publish_rate_hz',   10.0)
        # Detector imperfections (off by default). A real YOLO stream drops true
        # probes (dropout) and fires on ground clutter (false positives). These
        # are what the filter's gating + persistence must survive but a plain
        # average cannot — the decisive real-world test.
        self.declare_parameter('detection_miss_prob',        0.0)   # P(drop a true detection)
        self.declare_parameter('false_detections_per_frame', 0.0)   # Poisson mean of clutter
        # D435i depth noise model: σ_z = coeff * z²  (stereo disparity grows quadratically)
        # 0.0015 → ~9 mm RMS at 2.5 m, consistent with D435i field measurements
        self.declare_parameter('depth_noise_coeff', 0.0015)
        # D435i operating range: 0.2 m – 4.0 m (beyond this the sensor returns 0/invalid)
        self.declare_parameter('depth_min_range_m', 0.2)
        self.declare_parameter('depth_max_range_m', 4.0)
        # Drone pose position noise: 1-σ (RTK-level). Vertical is worse than
        # horizontal, so eph (xy) and epv (z) are independent.
        self.declare_parameter('pose_pos_noise_m',   0.02)   # eph (horizontal)
        self.declare_parameter('pose_pos_noise_z_m', 0.035)  # epv (vertical)
        # Attitude estimate noise (EKF2-like, 1-σ): tilt = roll/pitch, yaw separate.
        # Tilt is well-observed (gravity reference); yaw is weaker (mag/GPS heading).
        self.declare_parameter('pose_tilt_noise_deg', 0.5)
        self.declare_parameter('pose_yaw_noise_deg',  1.0)
        # Attitude error is temporally correlated (slow estimator bias), not white:
        # an AR(1) with this per-frame retention. Stationary variance is preserved,
        # so the reported covariance stays consistent. 0 = white, →1 = slow drift.
        self.declare_parameter('pose_att_correlation', 0.9)
        # ArUco markers [id, x, y, z, yaw_deg, ...] — lie flat on the ground (normal +z).
        self.declare_parameter('aruco_markers',
                               [0., 0.0, -1.5, 0.0, 30.0,   1., 2.0, 0.5, 0.0, -60.0])
        self.declare_parameter('aruco_pos_noise_m',   0.02)  # PnP position 1-σ
        self.declare_parameter('aruco_rot_noise_deg', 1.5)   # PnP rotation 1-σ
        self.declare_parameter('aruco_max_range_m',   4.0)

        self._noise             = self.get_parameter('detection_noise_px').value
        self._depth_noise_coeff = self.get_parameter('depth_noise_coeff').value
        self._depth_min_m       = self.get_parameter('depth_min_range_m').value
        self._depth_max_m       = self.get_parameter('depth_max_range_m').value
        self._pose_noise_m      = self.get_parameter('pose_pos_noise_m').value
        self._pose_noise_z      = self.get_parameter('pose_pos_noise_z_m').value
        self._tilt_std          = np.deg2rad(self.get_parameter('pose_tilt_noise_deg').value)
        self._yaw_std           = np.deg2rad(self.get_parameter('pose_yaw_noise_deg').value)
        self._att_corr          = float(self.get_parameter('pose_att_correlation').value)
        self._aruco_pos_std     = self.get_parameter('aruco_pos_noise_m').value
        self._aruco_rot_std     = np.deg2rad(self.get_parameter('aruco_rot_noise_deg').value)
        self._aruco_max_m       = self.get_parameter('aruco_max_range_m').value
        self._miss_prob         = float(self.get_parameter('detection_miss_prob').value)
        self._false_rate        = float(self.get_parameter('false_detections_per_frame').value)
        self._n_false_total     = 0
        self._n_missed_total    = 0
        rate = self.get_parameter('publish_rate_hz').value

        self._waypoints = _lawnmower(
            self.get_parameter('scan_x_min').value,
            self.get_parameter('scan_x_max').value,
            self.get_parameter('scan_y_min').value,
            self.get_parameter('scan_y_max').value,
            self.get_parameter('scan_altitude').value,
            self.get_parameter('scan_step').value,
        )
        self._wp_idx    = 0
        self._scan_done = False
        self._last_heading = 0.0  # radians; default: face East
        self._att_err   = np.zeros(3)   # AR(1) attitude-error state [roll, pitch, yaw]

        # ── ground-truth ArUco markers: parse [id, x, y, z, yaw_deg] rows ───────
        am = np.array(self.get_parameter('aruco_markers').value,
                      dtype=np.float64).reshape(-1, 5)
        self._aruco_gt = []   # list of (id:int, pos:(3,), R_marker_to_world:(3,3))
        for row in am:
            yaw = np.deg2rad(row[4])
            cy, sy = np.cos(yaw), np.sin(yaw)
            R_mw = np.array([[cy, -sy, 0.], [sy, cy, 0.], [0., 0., 1.]])
            self._aruco_gt.append((int(round(row[0])), row[1:4].copy(), R_mw))
        self._aruco_estimates = []   # [(id, pos(3), quat(4) wxyz, std(3)), ...]

        # ── pre-bake pixel ray grid (H, W, 3) in camera frame ───────────────
        # Ray for pixel (u,v): [(u-cx)/fx, (v-cy)/fy, 1]
        # z=1 so the intersection parameter t equals camera z-depth directly.
        ug, vg = np.meshgrid(np.arange(self._W, dtype=np.float32),
                              np.arange(self._H, dtype=np.float32))
        self._rays_cam = np.stack([
            (ug - self._cx) / self._fx,
            (vg - self._cy) / self._fy,
            np.ones((self._H, self._W), dtype=np.float32),
        ], axis=2)  # (H, W, 3)

        # ── shared plot state (written by ROS thread, read by main thread) ──
        self._drone_trail = deque(maxlen=120)  # (x, y)
        self._meas_trail  = deque(maxlen=60)   # np.ndarray world-frame 3-D points
        self._false_trail = deque(maxlen=60)   # np.ndarray world-frame clutter points
        self._estimates   = []                 # list of (x, y, z, sx, sy, sz)
        self._footprint_xy = None              # (4,2) camera ground footprint, XY

        # Naive baseline: per-GT-probe running mean of the world-frame
        # measurements (the red dots), accumulated over the whole scan. Uses
        # oracle GT association — the best case for a plain average — so the
        # Kalman filter is benchmarked against the easiest possible rival.
        self._avg_sum   = np.zeros((len(self._gt), 3))
        self._avg_count = np.zeros(len(self._gt), dtype=np.int64)

        # ── ROS I/O ──────────────────────────────────────────────────────────
        qos = QoSProfile(depth=5, reliability=QoSReliabilityPolicy.BEST_EFFORT)

        self._pub_det  = self.create_publisher(ProbeDetections, 'probe_detector/detections', qos)
        self._pub_dep  = self.create_publisher(Image,           'out/cam/synced/depth',      qos)
        self._pub_pose = self.create_publisher(PoseWithCovarianceStamped, 'out/cam/synced/pose',  qos)
        self._pub_gt    = self.create_publisher(ProbeLocations,  'probe_filter_sim/ground_truth', 10)
        self._pub_aruco = self.create_publisher(ArucoDetections, 'aruco_detector/detections', qos)
        self.create_subscription(
            ProbeLocations, 'probe_detector/locations', self._on_locations, 10)
        self.create_subscription(
            ArucoLocations, 'aruco_detector/locations', self._on_aruco_locations, 10)

        self._timer = self.create_timer(1.0 / rate, self._tick)
        self.get_logger().info(
            f'probe_filter_sim ready — {len(self._waypoints)} waypoints, '
            f'{len(self._gt)} probe(s)')

        self._setup_plot()

    # ── plot setup ───────────────────────────────────────────────────────────

    def _setup_plot(self):
        plt.ion()
        self._fig, self._ax = plt.subplots(figsize=(9, 9))
        ax = self._ax
        ax.set_aspect('equal')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        ax.grid(True, alpha=0.3)

        x_min = self.get_parameter('scan_x_min').value
        x_max = self.get_parameter('scan_x_max').value
        y_min = self.get_parameter('scan_y_min').value
        y_max = self.get_parameter('scan_y_max').value
        ax.set_xlim(x_min - 0.5, x_max + 0.5)
        ax.set_ylim(y_min - 0.5, y_max + 0.5)

        # Static: ground-truth probes
        for i, p in enumerate(self._gt):
            ax.plot(p[0], p[1], '*', color='gold', markersize=20, zorder=5,
                    markeredgecolor='goldenrod', markeredgewidth=0.5)
            ax.annotate(f'GT{i}  ({p[0]:.1f}, {p[1]:.1f})',
                        (p[0], p[1]), textcoords='offset points',
                        xytext=(8, 6), fontsize=8, color='goldenrod')

        # Static: ground-truth ArUco markers (square + yaw tick)
        for mid, m_pos, R_mw in self._aruco_gt:
            ax.plot(m_pos[0], m_pos[1], 's', color='mediumpurple', markersize=11,
                    zorder=5, markeredgecolor='indigo', markeredgewidth=0.6)
            xaxis = R_mw[:, 0]   # marker x-axis in world → yaw direction
            ax.plot([m_pos[0], m_pos[0] + 0.3 * xaxis[0]],
                    [m_pos[1], m_pos[1] + 0.3 * xaxis[1]],
                    '-', color='indigo', lw=1.2, zorder=5)
            ax.annotate(f'AR{mid}', (m_pos[0], m_pos[1]), textcoords='offset points',
                        xytext=(8, -12), fontsize=8, color='indigo')

        # Dynamic: camera ground footprint (range-clipped field of view)
        self._footprint = mpatches.Polygon(
            np.zeros((4, 2)), closed=True, fill=True,
            facecolor='steelblue', alpha=0.12,
            edgecolor='steelblue', linewidth=1.0, linestyle='--', zorder=2)
        self._footprint.set_visible(False)
        ax.add_patch(self._footprint)

        # Dynamic: fading drone trail + current position marker
        self._trail_line, = ax.plot([], [], '-', color='steelblue', alpha=0.25, lw=1)
        self._drone_dot,  = ax.plot([], [], '^', color='steelblue', markersize=10, zorder=6)

        # Dynamic: scatter of recent measurements
        self._meas_sc = ax.scatter([], [], c='tomato', s=20, alpha=0.6, zorder=3)

        # Dynamic: scatter of recent clutter (false) detections
        self._false_sc = ax.scatter([], [], c='black', marker='x', s=30,
                                    alpha=0.7, linewidths=1.2, zorder=3)

        # Estimate artists are rebuilt on every update (at most a handful each)
        self._est_artists       = []
        self._aruco_est_artists = []
        self._avg_artists       = []

        ax.legend(handles=[
            Line2D([0],[0], marker='*', color='w', markerfacecolor='gold',
                   markersize=14, label='Probe GT'),
            Line2D([0],[0], marker='s', color='w', markerfacecolor='mediumpurple',
                   markersize=10, label='ArUco GT'),
            Line2D([0],[0], marker='^', color='steelblue',
                   markersize=10, label='Drone'),
            mpatches.Patch(facecolor='steelblue', alpha=0.25,
                           edgecolor='steelblue', linestyle='--',
                           label='Camera footprint'),
            Line2D([0],[0], marker='o', color='w', markerfacecolor='tomato',
                   markersize=8, alpha=0.6, label='Probe measurements'),
            Line2D([0],[0], marker='x', color='black', linestyle='None',
                   markersize=8, label='Clutter (false)'),
            Line2D([0],[0], marker='o', color='w', markerfacecolor='limegreen',
                   markersize=10, label='Probe estimate'),
            Line2D([0],[0], marker='D', color='w', markerfacecolor='darkorange',
                   markersize=9, label='Probe avg (baseline)'),
            Line2D([0],[0], marker='s', color='w', markerfacecolor='magenta',
                   markersize=9, label='ArUco estimate'),
        ], loc='upper right', fontsize=8)

        plt.tight_layout()

    # ── plot update (called from main thread) ─────────────────────────────────

    def update_plot(self):
        ax = self._ax

        # Title: live progress
        n, total = self._wp_idx, len(self._waypoints)
        n_est = len(self._estimates)
        if self._scan_done:
            ax.set_title(f'Probe Filter — scan complete  ({n_est} probe(s) found)')
        else:
            ax.set_title(f'Probe Filter — wp {n}/{total}  |  {n_est} probe(s) tracked')

        # Camera ground footprint (where the forward-tilted camera is looking)
        fp = self._footprint_xy
        if fp is not None:
            self._footprint.set_xy(fp)
            self._footprint.set_visible(True)

        # Drone trail
        trail = list(self._drone_trail)
        if trail:
            xs, ys = zip(*trail)
            self._trail_line.set_data(xs, ys)
            self._drone_dot.set_data([xs[-1]], [ys[-1]])

        # Recent measurements
        meas = list(self._meas_trail)
        if meas:
            pts = np.array(meas)
            self._meas_sc.set_offsets(pts[:, :2])
        else:
            self._meas_sc.set_offsets(np.empty((0, 2)))

        # Recent clutter (false) detections
        false_pts = list(self._false_trail)
        if false_pts:
            self._false_sc.set_offsets(np.array(false_pts)[:, :2])
        else:
            self._false_sc.set_offsets(np.empty((0, 2)))

        # Filter estimates + σ rings
        for artist in self._est_artists:
            artist.remove()
        self._est_artists.clear()

        for (x, y, _z, sx, sy, _sz) in list(self._estimates):
            dot, = ax.plot(x, y, 'o', color='limegreen', markersize=10, zorder=4)
            ring = mpatches.Circle((x, y), radius=max(sx, sy),
                                   fill=False, edgecolor='limegreen',
                                   linewidth=1.5, alpha=0.8)
            ax.add_patch(ring)
            self._est_artists.extend([dot, ring])

        # Naive-average baseline: per-GT-probe running mean of the measurements
        for artist in self._avg_artists:
            artist.remove()
        self._avg_artists.clear()

        counts = self._avg_count.copy()
        sums   = self._avg_sum.copy()
        for i in range(len(self._gt)):
            if counts[i] == 0:
                continue
            m = sums[i] / counts[i]
            dot, = ax.plot(m[0], m[1], 'D', color='darkorange', markersize=9,
                           zorder=4, markeredgecolor='saddlebrown',
                           markeredgewidth=0.6)
            self._avg_artists.append(dot)

        # ArUco estimates + σ rings + yaw ticks
        for artist in self._aruco_est_artists:
            artist.remove()
        self._aruco_est_artists.clear()

        for (_mid, p, q, s) in list(self._aruco_estimates):
            yaw = np.arctan2(2.0 * (q[0]*q[3] + q[1]*q[2]),
                             1.0 - 2.0 * (q[2]**2 + q[3]**2))
            dot,  = ax.plot(p[0], p[1], 's', color='magenta', markersize=9, zorder=4)
            ring  = mpatches.Circle((p[0], p[1]), radius=max(s[0], s[1]),
                                    fill=False, edgecolor='magenta',
                                    linewidth=1.3, alpha=0.8)
            ax.add_patch(ring)
            tick, = ax.plot([p[0], p[0] + 0.3 * np.cos(yaw)],
                            [p[1], p[1] + 0.3 * np.sin(yaw)],
                            '-', color='magenta', lw=1.0, zorder=4)
            self._aruco_est_artists.extend([dot, ring, tick])

        self._fig.canvas.draw_idle()
        self._fig.canvas.flush_events()

    # ── depth image ──────────────────────────────────────────────────────────

    def _make_depth_image(self, drone_pos: np.ndarray, R_dw: np.ndarray, stamp) -> Image:
        """
        Flat-ground (z_world = 0) depth image encoded as 16UC1 millimetres.

        For every pixel, the camera ray is:
            p_world(t) = cam_origin_world + t * d_world
        where t is also the camera z-depth (because rays are defined with z_cam=1).
        Setting p_world.z = 0 and solving for t gives the ground depth directly.
        """
        R_wc = R_dw @ self._R_cd
        cam_origin = R_dw @ self._t_cd + drone_pos

        rays_world = self._rays_cam @ R_wc.T
        denom = rays_world[:, :, 2]

        with np.errstate(divide='ignore', invalid='ignore'):
            depth_z = np.where(denom < -1e-4, -cam_origin[2] / denom, 0.0)

        # D435i noise: σ_z = coeff * z²
        if self._depth_noise_coeff > 0.0:
            sigma_m = self._depth_noise_coeff * depth_z ** 2
            depth_z = depth_z + np.random.normal(0.0, sigma_m, depth_z.shape)

        # D435i operating range: pixels outside [min, max] are returned as 0 (invalid)
        depth_z = np.where(
            (depth_z >= self._depth_min_m) & (depth_z <= self._depth_max_m),
            depth_z, 0.0)

        z_mm = np.clip(depth_z * 1000.0, 0, 65535).astype(np.uint16)

        msg = Image()
        msg.header.stamp    = stamp
        msg.header.frame_id = 'camera'
        msg.height          = self._H
        msg.width           = self._W
        msg.encoding        = '16UC1'
        msg.is_bigendian    = False
        msg.step            = self._W * 2
        msg.data            = z_mm.tobytes()
        return msg

    # ── camera footprint ───────────────────────────────────────────────────────

    def _camera_footprint(self, drone_pos: np.ndarray, R_dw: np.ndarray) -> np.ndarray:
        """
        Ground-plane (XY) footprint of the camera's field of view, clipped to the
        depth operating range — the patch of ground where the camera can actually
        measure probes. The four image corners are cast as rays and intersected
        with z_world = 0, but the camera-z depth t is clamped to
        [depth_min, depth_max], so the far edge is the range cutoff, not infinity.
        """
        R_wc       = R_dw @ self._R_cd
        cam_origin = R_dw @ self._t_cd + drone_pos
        corners_px = ((0.0, 0.0), (self._W - 1.0, 0.0),
                      (self._W - 1.0, self._H - 1.0), (0.0, self._H - 1.0))

        pts = []
        for u, v in corners_px:
            ray_cam   = np.array([(u - self._cx) / self._fx,
                                  (v - self._cy) / self._fy, 1.0])
            ray_world = R_wc @ ray_cam
            # t is the camera-z depth where the ray meets the ground (rays z_cam=1)
            if ray_world[2] < -1e-6:
                t = -cam_origin[2] / ray_world[2]
            else:
                t = self._depth_max_m            # ray at/above horizon → no ground hit
            t = float(np.clip(t, self._depth_min_m, self._depth_max_m))
            pts.append((cam_origin + t * ray_world)[:2])
        return np.array(pts)

    # ── detections ───────────────────────────────────────────────────────────

    def _make_detections(self, drone_pos: np.ndarray, R_dw: np.ndarray,
                         depth_data: bytes, stamp) -> tuple:
        """
        Project each GT probe into the image plane; add Gaussian pixel noise.

        Also samples the noisy depth image at each detection pixel and
        back-projects to a drone-body-frame 3-D point. The caller lifts these to
        the world frame with the NOISY pose (mirroring the filter exactly), so
        the plotted 'measurements' show the real pose-induced spread.

        True detections may be randomly dropped (detector dropout) and random
        ground-clutter FALSE detections may be appended (Poisson count), so the
        filter sees the same imperfect stream a real YOLO produces.

        Returns (ProbeDetections, meas_body, meas_idx, false_body):
          meas_body  — body-frame 3-D points of TRUE detections
          meas_idx   — parallel GT probe index of each (oracle association for
                       the average baseline; false detections are NOT included)
          false_body — body-frame 3-D points of clutter detections, for plotting
        """
        R_wd = R_dw.T
        R_dc = self._R_cd.T
        depth_arr = np.frombuffer(depth_data, dtype=np.uint16).reshape(self._H, self._W)

        msg = ProbeDetections()
        msg.header.stamp    = stamp
        msg.header.frame_id = 'camera'
        meas_body = []
        meas_idx  = []

        for gt_idx, probe in enumerate(self._gt):
            p_body = R_wd @ (probe - drone_pos)
            p_cam  = R_dc @ (p_body - self._t_cd)

            if p_cam[2] <= 0.05:
                continue

            u = self._fx * p_cam[0] / p_cam[2] + self._cx + np.random.normal(0.0, self._noise)
            v = self._fy * p_cam[1] / p_cam[2] + self._cy + np.random.normal(0.0, self._noise)

            if not (0 <= u < self._W and 0 <= v < self._H):
                continue

            # Detector dropout: a would-be detection randomly suppressed.
            if self._miss_prob > 0.0 and np.random.random() < self._miss_prob:
                self._n_missed_total += 1
                continue

            msg.centroid_x.append(float(u))
            msg.centroid_y.append(float(v))
            # Range-dependent confidence (informational — the filter ignores it):
            # detection quality degrades with distance, as a real detector does.
            conf = np.clip(0.95 - 0.08 * max(0.0, p_cam[2] - 1.0)
                           + np.random.normal(0.0, 0.02), 0.5, 0.99)
            msg.confidence.append(float(conf))
            msg.num_detections += 1

            # Back-project via noisy depth — mirrors the filter's own projection
            iu = int(np.clip(round(u), 0, self._W - 1))
            iv = int(np.clip(round(v), 0, self._H - 1))
            d_mm = depth_arr[iv, iu]
            if d_mm > 0:
                z_d = d_mm / 1000.0
                p_cam_3d = np.array([(u - self._cx) * z_d / self._fx,
                                     (v - self._cy) * z_d / self._fy, z_d])
                p_body_3d = self._R_cd @ p_cam_3d + self._t_cd
                meas_body.append(p_body_3d)
                meas_idx.append(gt_idx)

        # False positives: clutter at random ground pixels (Poisson count). They
        # back-project through the real depth, so they look exactly like a probe
        # detection — the filter has no GT to tell them apart. NOT added to the
        # oracle average (which knows they are false); they only feed the filter.
        false_body = []
        n_false = np.random.poisson(self._false_rate) if self._false_rate > 0.0 else 0
        for _ in range(int(n_false)):
            u = np.random.uniform(0.0, self._W)
            v = np.random.uniform(0.0, self._H)
            iu = int(np.clip(round(u), 0, self._W - 1))
            iv = int(np.clip(round(v), 0, self._H - 1))
            d_mm = depth_arr[iv, iu]
            if d_mm == 0:
                continue   # no valid ground here (out of range) — detector wouldn't fire
            msg.centroid_x.append(float(u))
            msg.centroid_y.append(float(v))
            msg.confidence.append(float(np.clip(np.random.normal(0.7, 0.1), 0.5, 0.99)))
            msg.num_detections += 1
            z_d = d_mm / 1000.0
            p_cam_3d = np.array([(u - self._cx) * z_d / self._fx,
                                 (v - self._cy) * z_d / self._fy, z_d])
            false_body.append(self._R_cd @ p_cam_3d + self._t_cd)
        self._n_false_total += len(false_body)

        return msg, meas_body, meas_idx, false_body

    # ── ArUco detections ───────────────────────────────────────────────────────

    def _make_aruco_detections(self, drone_pos: np.ndarray, R_dw: np.ndarray,
                               stamp) -> ArucoDetections:
        """
        Project each GT marker into the camera frame as (tvec, rvec) with PnP
        noise. Imaged from the TRUE pose; the filter lifts to world with the
        noisy pose, so the mismatch drives the 6-DoF ArUco measurement noise R.
        """
        R_wd = R_dw.T
        R_dc = self._R_cd.T
        R_cw = R_dw @ self._R_cd          # camera → world

        msg = ArucoDetections()
        msg.header.stamp    = stamp
        msg.header.frame_id = 'camera'

        for mid, m_pos, R_mw in self._aruco_gt:
            p_cam = R_dc @ (R_wd @ (m_pos - drone_pos) - self._t_cd)
            if p_cam[2] <= 0.05 or p_cam[2] > self._aruco_max_m:
                continue
            u = self._fx * p_cam[0] / p_cam[2] + self._cx
            v = self._fy * p_cam[1] / p_cam[2] + self._cy
            if not (0 <= u < self._W and 0 <= v < self._H):
                continue

            # Position: tvec in camera frame + PnP position noise
            tvec = p_cam + np.random.normal(0.0, self._aruco_pos_std, 3)
            # Orientation: marker→camera, perturbed by a small PnP rotation
            R_mc = _rvec_to_rot(np.random.normal(0.0, self._aruco_rot_std, 3)) \
                   @ (R_cw.T @ R_mw)
            rvec = _rot_to_rvec(R_mc)

            msg.marker_id.append(int(mid))
            msg.tvec.extend([float(tvec[0]), float(tvec[1]), float(tvec[2])])
            msg.rvec.extend([float(rvec[0]), float(rvec[1]), float(rvec[2])])
            msg.num_detections += 1

        return msg

    # ── main tick ────────────────────────────────────────────────────────────

    def _tick(self):
        if self._wp_idx >= len(self._waypoints):
            self._timer.cancel()
            self._scan_done = True
            self._print_summary()
            return

        x, y, z = self._waypoints[self._wp_idx]
        self._wp_idx += 1

        drone_pos = np.array([x, y, z], dtype=np.float64)

        # Drone faces the direction of travel so the forward-tilted camera looks ahead.
        if self._wp_idx >= 2:
            prev = self._waypoints[self._wp_idx - 2]
            dx, dy = x - prev[0], y - prev[1]
            if abs(dx) > 1e-9 or abs(dy) > 1e-9:
                self._last_heading = np.arctan2(dy, dx)
        heading = self._last_heading
        c, s = np.cos(heading), np.sin(heading)
        # R_dw = world_R_drone: columns are drone body axes in world frame
        R_dw = np.array([[c, -s, 0.],
                          [s,  c, 0.],
                          [0., 0., 1.]])

        stamp = self.get_clock().now().to_msg()

        # Depth + detections are imaged from the TRUE pose — the camera sees truth.
        dep        = self._make_depth_image(drone_pos, R_dw, stamp)
        det, meas_body, meas_idx, false_body = self._make_detections(
            drone_pos, R_dw, bytes(dep.data), stamp)
        aruco_det  = self._make_aruco_detections(drone_pos, R_dw, stamp)

        # The pose published to the filter carries realistic estimator noise:
        # RTK-level position (eph horizontal, larger epv vertical) + EKF-like
        # attitude error. Attitude error is a slow AR(1) drift (not white), so
        # consecutive frames see a similar tilt — as a real estimator does. The
        # mismatch vs the true pose is exactly what R must cover (position via the
        # lever arm, orientation directly).
        eph, epv  = self._pose_noise_m, self._pose_noise_z
        noisy_pos = drone_pos + np.random.normal(0.0, [eph, eph, epv])
        sigma_att = np.array([self._tilt_std, self._tilt_std, self._yaw_std])
        phi       = self._att_corr
        self._att_err = phi * self._att_err + np.sqrt(1.0 - phi * phi) * \
                        np.random.normal(0.0, sigma_att)
        roll_err, pitch_err, yaw_err = self._att_err
        quat      = _euler_to_quat(roll_err, pitch_err, heading + yaw_err)
        pos_var   = (eph ** 2, eph ** 2, epv ** 2)
        att_var   = tuple(sigma_att ** 2)
        pose      = _make_pose_msg(noisy_pos, quat, stamp, pos_var, att_var)

        # Lift the probe measurements to world with the NOISY pose, as the filter does.
        R_noisy  = _quat_to_rot(*quat)
        meas_3d  = [R_noisy @ pb + noisy_pos for pb in meas_body]
        false_3d = [R_noisy @ pb + noisy_pos for pb in false_body]

        # Naive baseline: fold each measurement into its GT probe's running mean.
        for gt_idx, pt in zip(meas_idx, meas_3d):
            self._avg_sum[gt_idx]   += pt
            self._avg_count[gt_idx] += 1

        gt = _make_locations_msg(self._gt, stamp)

        self._pub_det.publish(det)
        self._pub_aruco.publish(aruco_det)
        self._pub_dep.publish(dep)
        self._pub_pose.publish(pose)
        self._pub_gt.publish(gt)

        self._footprint_xy = self._camera_footprint(drone_pos, R_dw)
        self._drone_trail.append((x, y))
        self._meas_trail.extend(meas_3d)
        self._false_trail.extend(false_3d)

    # ── filter output callback ───────────────────────────────────────────────

    def _on_locations(self, msg: ProbeLocations):
        if msg.num_probes == 0:
            return
        estimated = np.array(msg.positions,   dtype=np.float64).reshape(-1, 3)
        unc       = np.array(msg.uncertainty, dtype=np.float64).reshape(-1, 3)
        self._estimates = [
            (est[0], est[1], est[2], sig[0], sig[1], sig[2])
            for est, sig in zip(estimated, unc)
        ]

    def _on_aruco_locations(self, msg: ArucoLocations):
        if msg.num_markers == 0:
            self._aruco_estimates = []
            return
        pos = np.array(msg.position,    dtype=np.float64).reshape(-1, 3)
        qua = np.array(msg.orientation, dtype=np.float64).reshape(-1, 4)  # wxyz
        unc = np.array(msg.uncertainty, dtype=np.float64).reshape(-1, 3)
        self._aruco_estimates = [
            (int(mid), p, q, s)
            for mid, p, q, s in zip(msg.marker_id, pos, qua, unc)
        ]

    def _print_summary(self):
        lines = [f'Scan complete — {len(self._waypoints)} waypoints, '
                 f'{len(self._estimates)} probe(s) found of {len(self._gt)} GT']
        if self._false_rate > 0.0 or self._miss_prob > 0.0:
            n_ghost = max(0, len(self._estimates) - len(self._gt))
            lines.append(
                f'Clutter — injected {self._n_false_total} false detection(s), '
                f'dropped {self._n_missed_total} true detection(s)  →  '
                f'{n_ghost} ghost track(s) survived to publish')
        for est_tuple in self._estimates:
            est   = np.array(est_tuple[:3])
            sigma = np.array(est_tuple[3:])
            dists = np.linalg.norm(self._gt - est, axis=1)
            idx   = int(np.argmin(dists))
            lines.append(
                f'  GT[{idx}] ({self._gt[idx,0]:.1f},{self._gt[idx,1]:.1f},{self._gt[idx,2]:.1f})'
                f'  →  est ({est[0]:.3f},{est[1]:.3f},{est[2]:.3f})'
                f'  err={dists[idx]:.4f} m  σ=({sigma[0]:.3f},{sigma[1]:.3f},{sigma[2]:.3f})')

        # Naive-average baseline (oracle GT association) — is the filter worth it?
        lines.append('Naive average baseline (oracle GT association):')
        ests = (np.array([e[:3] for e in self._estimates])
                if self._estimates else np.empty((0, 3)))
        for i, p in enumerate(self._gt):
            if self._avg_count[i] == 0:
                lines.append(f'  GT[{i}] — no measurements')
                continue
            m   = self._avg_sum[i] / self._avg_count[i]
            err = float(np.linalg.norm(p - m))
            comp = ''
            if len(ests):
                f_err   = float(np.min(np.linalg.norm(ests - p, axis=1)))
                verdict = 'avg better' if err < f_err else 'filter better'
                comp    = f'  vs filter err={f_err:.4f} m → {verdict}'
            lines.append(
                f'  GT[{i}] avg over {int(self._avg_count[i])} meas → '
                f'({m[0]:.3f},{m[1]:.3f},{m[2]:.3f})  err={err:.4f} m{comp}')

        if self._aruco_gt:
            lines.append(f'ArUco — {len(self._aruco_estimates)} of '
                         f'{len(self._aruco_gt)} marker(s) tracked')
            gt_by_id = {mid: (pos, R_mw) for mid, pos, R_mw in self._aruco_gt}
            for (mid, p, q, s) in self._aruco_estimates:
                if mid not in gt_by_id:
                    continue
                gt_pos, gt_R = gt_by_id[mid]
                pos_err = float(np.linalg.norm(gt_pos - np.asarray(p)))
                rot_err = float(np.degrees(np.linalg.norm(
                    _rot_to_rvec(gt_R.T @ _quat_to_rot(*q)))))
                lines.append(
                    f'  AR{mid}  pos_err={pos_err:.4f} m  rot_err={rot_err:.2f}°'
                    f'  σ=({s[0]:.3f},{s[1]:.3f},{s[2]:.3f})')
        self.get_logger().info('\n'.join(lines))


# ── helpers ──────────────────────────────────────────────────────────────────

def _lawnmower(x_min, x_max, y_min, y_max, alt, step) -> list:
    wps, forward = [], True
    y = y_min
    while y <= y_max + 1e-9:
        xs = np.arange(x_min, x_max + 1e-9, step)
        for x in (xs if forward else reversed(xs)):
            wps.append((float(x), float(y), float(alt)))
        y += step
        forward = not forward
    return wps


def _euler_to_quat(roll, pitch, yaw):
    """Intrinsic Z-Y-X (yaw-pitch-roll) Euler angles → (w, x, y, z)."""
    cr, sr = np.cos(roll * 0.5),  np.sin(roll * 0.5)
    cp, sp = np.cos(pitch * 0.5), np.sin(pitch * 0.5)
    cy, sy = np.cos(yaw * 0.5),   np.sin(yaw * 0.5)
    return (cr * cp * cy + sr * sp * sy,    # w
            sr * cp * cy - cr * sp * sy,    # x
            cr * sp * cy + sr * cp * sy,    # y
            cr * cp * sy - sr * sp * cy)    # z


def _make_pose_msg(pos, quat, stamp, pos_var, att_var) -> PoseWithCovarianceStamped:
    msg = PoseWithCovarianceStamped()
    msg.header.stamp    = stamp
    msg.header.frame_id = 'world'
    msg.pose.pose.position.x = float(pos[0])
    msg.pose.pose.position.y = float(pos[1])
    msg.pose.pose.position.z = float(pos[2])
    msg.pose.pose.orientation.w = float(quat[0])
    msg.pose.pose.orientation.x = float(quat[1])
    msg.pose.pose.orientation.y = float(quat[2])
    msg.pose.pose.orientation.z = float(quat[3])
    # Report the injected noise as a 6x6 cov [x,y,z, roll,pitch,yaw], matching
    # camera_relay's layout so the filter sees one convention in sim and on HW.
    cov = [0.0] * 36
    cov[0],  cov[7],  cov[14] = float(pos_var[0]), float(pos_var[1]), float(pos_var[2])
    cov[21], cov[28], cov[35] = float(att_var[0]), float(att_var[1]), float(att_var[2])
    msg.pose.covariance = cov
    return msg


def _make_locations_msg(probes: np.ndarray, stamp) -> ProbeLocations:
    msg = ProbeLocations()
    msg.header.stamp    = stamp
    msg.header.frame_id = 'world'
    for p in probes:
        msg.positions.extend([float(p[0]), float(p[1]), float(p[2])])
        msg.uncertainty.extend([0.0, 0.0, 0.0])
        msg.num_probes += 1
    return msg


def main(args=None):
    rclpy.init(args=args)
    node = ProbeFilterSim()

    # ROS spin in background; matplotlib event loop stays on the main thread
    spin_thread = threading.Thread(target=rclpy.spin, args=(node,), daemon=True)
    spin_thread.start()

    try:
        while rclpy.ok() and not node._scan_done:
            node.update_plot()
            plt.pause(0.05)   # ~20 Hz render + matplotlib event processing
        # Scan complete — freeze final state; window stays open for inspection
        if plt.get_fignums():
            node.get_logger().info('Close the plot window to exit.')
            plt.ioff()
            plt.show(block=True)
    except KeyboardInterrupt:
        pass
    finally:
        # Shut down first so rclpy.spin() in the daemon thread returns, then join
        # it before tearing the node down — avoids the rcl context being finalized
        # while the spin thread still holds it (the "terminate called" abort).
        rclpy.shutdown()
        spin_thread.join(timeout=2.0)
        node.destroy_node()


if __name__ == '__main__':
    main()
