#!/usr/bin/env python3
"""
probe_filter_sim — synthetic test node for probe_filter.

Bypasses the YOLO detector. Back-projects known ground-truth probe positions
into the camera frame to produce synthetic ProbeDetections, and renders a
realistic flat-ground (z_world = 0) depth image so the filter's full
3-D-lifting pipeline is exercised end-to-end.

The drone follows a lawnmower scan pattern at fixed altitude.

Topics published (all with matching stamps for ApproximateTime sync):
  probe_detector/detections       ProbeDetections  — synthetic pixel centroids
  out/cam/synced/depth            sensor_msgs/Image (16UC1, mm) — flat ground
  out/cam/synced/pose             geometry_msgs/PoseStamped     — drone pose
  probe_filter_sim/ground_truth   ProbeLocations   — GT positions for plotting

Subscribes:
  probe_detector/locations        ProbeLocations — filter output; logs error vs GT

A live matplotlib window shows the top-down XY view:
  ★ gold stars  — ground-truth probe positions
  ▲ blue arrow  — current drone position (fading trail)
  ● red dots    — noisy 3-D measurements back-projected from depth + pixel
  ● green dots  — filter estimates; ring = 1-σ horizontal uncertainty radius

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

from geometry_msgs.msg import PoseStamped
from sensor_msgs.msg import Image
from asr_comms.msg import ProbeDetections, ProbeLocations


def _quat_to_rot(qw: float, qx: float, qy: float, qz: float) -> np.ndarray:
    """Unit quaternion → 3×3 rotation matrix (world_R_body)."""
    n = np.sqrt(qw**2 + qx**2 + qy**2 + qz**2)
    qw, qx, qy, qz = qw/n, qx/n, qy/n, qz/n
    return np.array([
        [1-2*(qy**2+qz**2), 2*(qx*qy-qz*qw),   2*(qx*qz+qy*qw)],
        [2*(qx*qy+qz*qw),   1-2*(qx**2+qz**2), 2*(qy*qz-qx*qw)],
        [2*(qx*qz-qy*qw),   2*(qy*qz+qx*qw),   1-2*(qx**2+qy**2)],
    ], dtype=np.float64)


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
        # D435i depth noise model: σ_z = coeff * z²  (stereo disparity grows quadratically)
        # 0.0015 → ~9 mm RMS at 2.5 m, consistent with D435i field measurements
        self.declare_parameter('depth_noise_coeff', 0.0015)
        # D435i operating range: 0.2 m – 4.0 m (beyond this the sensor returns 0/invalid)
        self.declare_parameter('depth_min_range_m', 0.2)
        self.declare_parameter('depth_max_range_m', 4.0)
        # Drone pose position noise: 1-σ per axis (models GPS/VIO uncertainty)
        self.declare_parameter('pose_pos_noise_m', 0.02)

        self._noise             = self.get_parameter('detection_noise_px').value
        self._depth_noise_coeff = self.get_parameter('depth_noise_coeff').value
        self._depth_min_m       = self.get_parameter('depth_min_range_m').value
        self._depth_max_m       = self.get_parameter('depth_max_range_m').value
        self._pose_noise_m      = self.get_parameter('pose_pos_noise_m').value
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
        self._estimates   = []                 # list of (x, y, z, sx, sy, sz)

        # ── ROS I/O ──────────────────────────────────────────────────────────
        qos = QoSProfile(depth=5, reliability=QoSReliabilityPolicy.BEST_EFFORT)

        self._pub_det  = self.create_publisher(ProbeDetections, 'probe_detector/detections', qos)
        self._pub_dep  = self.create_publisher(Image,           'out/cam/synced/depth',      qos)
        self._pub_pose = self.create_publisher(PoseStamped,     'out/cam/synced/pose',        qos)
        self._pub_gt   = self.create_publisher(ProbeLocations,  'probe_filter_sim/ground_truth', 10)
        self.create_subscription(
            ProbeLocations, 'probe_detector/locations', self._on_locations, 10)

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

        # Dynamic: fading drone trail + current position marker
        self._trail_line, = ax.plot([], [], '-', color='steelblue', alpha=0.25, lw=1)
        self._drone_dot,  = ax.plot([], [], '^', color='steelblue', markersize=10, zorder=6)

        # Dynamic: scatter of recent measurements
        self._meas_sc = ax.scatter([], [], c='tomato', s=20, alpha=0.6, zorder=3)

        # Estimate artists are rebuilt on every update (at most a handful of probes)
        self._est_artists = []

        ax.legend(handles=[
            Line2D([0],[0], marker='*', color='w', markerfacecolor='gold',
                   markersize=14, label='Ground truth'),
            Line2D([0],[0], marker='^', color='steelblue',
                   markersize=10, label='Drone'),
            Line2D([0],[0], marker='o', color='w', markerfacecolor='tomato',
                   markersize=8, alpha=0.6, label='Measurements (noisy 3-D)'),
            Line2D([0],[0], marker='o', color='w', markerfacecolor='limegreen',
                   markersize=10, label='Filter estimate'),
            mpatches.Patch(facecolor='none', edgecolor='limegreen',
                           linewidth=1.5, label='1-σ uncertainty'),
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

    # ── detections ───────────────────────────────────────────────────────────

    def _make_detections(self, drone_pos: np.ndarray, R_dw: np.ndarray,
                         depth_data: bytes, stamp) -> tuple:
        """
        Project each GT probe into the image plane; add Gaussian pixel noise.

        Also samples the noisy depth image at each detection pixel and
        back-projects to a world-frame 3-D point — this is exactly what the
        filter receives, and what the live plotter shows as 'measurements'.

        Returns (ProbeDetections, list[np.ndarray]).
        """
        R_wd = R_dw.T
        R_dc = self._R_cd.T
        depth_arr = np.frombuffer(depth_data, dtype=np.uint16).reshape(self._H, self._W)

        msg = ProbeDetections()
        msg.header.stamp    = stamp
        msg.header.frame_id = 'camera'
        meas_3d = []

        for probe in self._gt:
            p_body = R_wd @ (probe - drone_pos)
            p_cam  = R_dc @ (p_body - self._t_cd)

            if p_cam[2] <= 0.05:
                continue

            u = self._fx * p_cam[0] / p_cam[2] + self._cx + np.random.normal(0.0, self._noise)
            v = self._fy * p_cam[1] / p_cam[2] + self._cy + np.random.normal(0.0, self._noise)

            if not (0 <= u < self._W and 0 <= v < self._H):
                continue

            msg.centroid_x.append(float(u))
            msg.centroid_y.append(float(v))
            msg.confidence.append(0.90)
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
                meas_3d.append(R_dw @ p_body_3d + drone_pos)

        return msg, meas_3d

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

        stamp     = self.get_clock().now().to_msg()

        dep          = self._make_depth_image(drone_pos, R_dw, stamp)
        det, meas_3d = self._make_detections(drone_pos, R_dw, bytes(dep.data), stamp)
        # Pose published to the filter has GPS-level noise; depth/detections use true position.
        noisy_pos = drone_pos + np.random.normal(0.0, self._pose_noise_m, 3)
        pose      = _make_pose_msg(*noisy_pos, heading, stamp)
        gt           = _make_locations_msg(self._gt, stamp)

        self._pub_det.publish(det)
        self._pub_dep.publish(dep)
        self._pub_pose.publish(pose)
        self._pub_gt.publish(gt)

        self._drone_trail.append((x, y))
        self._meas_trail.extend(meas_3d)

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

    def _print_summary(self):
        lines = [f'Scan complete — {len(self._waypoints)} waypoints, '
                 f'{len(self._estimates)} probe(s) found of {len(self._gt)} GT']
        for est_tuple in self._estimates:
            est   = np.array(est_tuple[:3])
            sigma = np.array(est_tuple[3:])
            dists = np.linalg.norm(self._gt - est, axis=1)
            idx   = int(np.argmin(dists))
            lines.append(
                f'  GT[{idx}] ({self._gt[idx,0]:.1f},{self._gt[idx,1]:.1f},{self._gt[idx,2]:.1f})'
                f'  →  est ({est[0]:.3f},{est[1]:.3f},{est[2]:.3f})'
                f'  err={dists[idx]:.4f} m  σ=({sigma[0]:.3f},{sigma[1]:.3f},{sigma[2]:.3f})')
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


def _make_pose_msg(x, y, z, heading_rad, stamp) -> PoseStamped:
    msg = PoseStamped()
    msg.header.stamp    = stamp
    msg.header.frame_id = 'world'
    msg.pose.position.x = float(x)
    msg.pose.position.y = float(y)
    msg.pose.position.z = float(z)
    # Pure yaw quaternion: rotate around world Z by heading angle
    msg.pose.orientation.w = float(np.cos(heading_rad / 2))
    msg.pose.orientation.z = float(np.sin(heading_rad / 2))
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
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
