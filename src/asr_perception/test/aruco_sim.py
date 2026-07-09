#!/usr/bin/env python3
"""
aruco_sim — synthetic test node for the ArUco detector (reference OR MATLAB).

Renders 640x480 images containing DICT_4X4_50 markers at KNOWN camera-frame
poses, publishes them on the camera topic, subscribes to the detector's output
and checks every detection against ground truth. It is detector-agnostic: it
talks only to the topics in aruco_detector_node.cpp, so the exact same test
validates the pure-OpenCV ``aruco_detector_ref`` and, once
``matlab/generate_code.m`` has been run, the MATLAB ``aruco_detector``.

Pipeline per frame:
  ground-truth (id, R_marker_to_cam, t_cam) -> project marker corners -> warp the
  marker bitmap (with a white quiet zone) onto a blank canvas -> publish Image.
The render is self-checked at startup against cv2.aruco so the ground truth is
correct by construction.

Publishes:  out/cam/synced/color        sensor_msgs/Image (rgb8, 640x480)
Subscribes: aruco_detector/detections   asr_comms/ArucoDetections

Checks per marker, matched to its frame by header stamp:
  - the marker id is detected,
  - tvec error (camera-frame position) < tvec_tol_mm,
  - orientation error < orient_tol_deg  (compared convention-robustly against
    both R and R^T, since rvec sign conventions differ between detectors; the
    actual convention seen is reported so a MATLAB rvec flip is still visible).

Important geometry notes (learned the hard way, see the inline comments):
  - A 150 mm marker is only readable at sub-metre range for fx~426 px
    (pixel size = fx * marker_m / z); poses sit at 0.45-0.8 m.
  - A marker FACING the camera has R_marker_to_cam = diag(1,-1,-1) (Rx 180),
    not identity; identity renders the marker upside-down (an invalid pattern).
  - A head-on marker's out-of-plane tilt is genuinely unobservable, so every
    test pose carries a real tilt to keep the orientation check meaningful.
"""

import os

import cv2
import numpy as np

import rclpy
from rclpy.node import Node
from rclpy.qos import (QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy)

from sensor_msgs.msg import Image
from asr_comms.msg import ArucoDetections


# --- small rotation helpers ------------------------------------------------
def _euler_R(roll_deg, pitch_deg, yaw_deg):
    rx, ry, rz = np.radians([roll_deg, pitch_deg, yaw_deg])
    Rx = np.array([[1, 0, 0], [0, np.cos(rx), -np.sin(rx)], [0, np.sin(rx), np.cos(rx)]])
    Ry = np.array([[np.cos(ry), 0, np.sin(ry)], [0, 1, 0], [-np.sin(ry), 0, np.cos(ry)]])
    Rz = np.array([[np.cos(rz), -np.sin(rz), 0], [np.sin(rz), np.cos(rz), 0], [0, 0, 1]])
    return Rz @ Ry @ Rx


def _rvec_to_R(rvec):
    R, _ = cv2.Rodrigues(np.asarray(rvec, dtype=np.float64).reshape(3, 1))
    return R


def _geodesic_deg(Ra, Rb):
    c = np.clip((np.trace(Ra.T @ Rb) - 1.0) * 0.5, -1.0, 1.0)
    return float(np.degrees(np.arccos(c)))


# Baseline "facing the camera": marker +z points back toward the camera.
_R_FACE = np.diag([1.0, -1.0, -1.0])


class ArucoSim(Node):
    def __init__(self):
        super().__init__('aruco_sim')

        self.declare_parameter('fx',             425.88)
        self.declare_parameter('fy',             425.88)
        self.declare_parameter('cx',             430.51)
        self.declare_parameter('cy',             238.53)
        self.declare_parameter('marker_size_mm', 150.0)
        self.declare_parameter('img_width',      640)
        self.declare_parameter('img_height',     480)
        self.declare_parameter('publish_rate_hz', 5.0)
        self.declare_parameter('tvec_tol_mm',    50.0)
        self.declare_parameter('orient_tol_deg', 12.0)
        # Min fraction of expected markers that must be detected to PASS. < 1.0
        # tolerates legitimate best_effort frame loss with the MATLAB node.
        self.declare_parameter('min_detect_frac', 0.9)
        # Number of full passes over the scene list before printing a final
        # summary and shutting down. 0 == run forever (good with a live launch).
        self.declare_parameter('cycles',         3)
        # Visualization: show a live window of each frame with ground-truth
        # marker outline + pose axes drawn on top (the OVERLAY is NOT published
        # to the detector — it only sees the clean image).
        self.declare_parameter('visualize',      False)
        # Render a grey "world" with a perspective floor grid instead of a blank
        # white canvas (purely cosmetic; the marker's white quiet zone sits on
        # top so detection is unaffected).
        self.declare_parameter('grid_background', True)
        # If non-empty, also write each annotated frame as a PNG here (useful
        # when there is no display, e.g. over SSH).
        self.declare_parameter('save_frames_dir', '')

        fx = self.get_parameter('fx').value
        fy = self.get_parameter('fy').value
        cx = self.get_parameter('cx').value
        cy = self.get_parameter('cy').value
        self._W = int(self.get_parameter('img_width').value)
        self._H = int(self.get_parameter('img_height').value)
        marker_m = self.get_parameter('marker_size_mm').value / 1000.0
        self._tvec_tol_mm = self.get_parameter('tvec_tol_mm').value
        self._orient_tol_deg = self.get_parameter('orient_tol_deg').value
        self._min_detect_frac = self.get_parameter('min_detect_frac').value
        self._cycles = int(self.get_parameter('cycles').value)
        self._visualize = bool(self.get_parameter('visualize').value)
        self._grid_bg = bool(self.get_parameter('grid_background').value)
        self._save_dir = self.get_parameter('save_frames_dir').value
        if self._save_dir:
            os.makedirs(self._save_dir, exist_ok=True)
        self._marker_m = marker_m
        self._frame_no = 0

        self._K = np.array([[fx, 0.0, cx], [0.0, fy, cy], [0.0, 0.0, 1.0]], dtype=np.float64)
        self._dist = np.zeros((5, 1), dtype=np.float64)
        self._bg = self._make_background()

        half = marker_m / 2.0
        self._obj = np.array([[-half,  half, 0.0],
                              [ half,  half, 0.0],
                              [ half, -half, 0.0],
                              [-half, -half, 0.0]], dtype=np.float32)

        self._dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
        self._detector = cv2.aruco.ArucoDetector(self._dict, cv2.aruco.DetectorParameters())

        # Scenes: each is a list of (id, R_marker_to_cam, t_cam[m]). Markers sit
        # sub-metre (readable size) and each carries a real out-of-plane tilt.
        self._scenes = [
            [(0,  _euler_R(18, 0, 0)   @ _R_FACE, np.array([0.00,  0.00, 0.50]))],
            [(3,  _euler_R(20, -15, 0) @ _R_FACE, np.array([0.12, -0.08, 0.60]))],
            [(7,  _euler_R(12, 22, 30) @ _R_FACE, np.array([-0.15, 0.05, 0.45]))],
            [(12, _euler_R(-25, 10, 0) @ _R_FACE, np.array([0.05,  0.10, 0.80]))],
            # two markers in one frame
            [(1,  _euler_R(15, -12, 0) @ _R_FACE, np.array([-0.18, -0.06, 0.55])),
             (2,  _euler_R(-18, 14, 0) @ _R_FACE, np.array([ 0.18,  0.06, 0.55]))],
        ]
        self._validate_scenes()

        # ground truth keyed by stamp -> {id: (R, t)}
        self._gt = {}
        self._scene_idx = 0
        self._published = 0
        # stats
        self._n_expected = 0
        self._n_detected = 0
        self._n_id_ok = 0
        self._n_tvec_ok = 0
        self._n_orient_ok = 0
        self._tvec_errs = []
        self._orient_errs = []
        self._conv_R = 0      # rvec matched R_marker_to_cam directly
        self._conv_Rt = 0     # rvec matched its transpose (flipped convention)
        self._done = False
        self._started = False

        # Publish RELIABLE: large image frames are dropped wholesale by
        # best_effort over localhost UDP. A reliable publisher is QoS-compatible
        # with both a reliable and a best_effort (MATLAB node) subscriber.
        pub_qos = QoSProfile(depth=5,
                             reliability=QoSReliabilityPolicy.RELIABLE,
                             history=QoSHistoryPolicy.KEEP_LAST)
        self._img_pub = self.create_publisher(Image, 'out/cam/synced/color', pub_qos)
        self._det_sub = self.create_subscription(
            ArucoDetections, 'aruco_detector/detections', self.on_detections, 10)

        rate = self.get_parameter('publish_rate_hz').value
        self._timer = self.create_timer(1.0 / rate, self.on_timer)
        self.get_logger().info(
            f'aruco_sim ready — {len(self._scenes)} scenes, '
            f'{"%d cycles" % self._cycles if self._cycles else "continuous"}, '
            f'tol {self._tvec_tol_mm:.0f} mm / {self._orient_tol_deg:.0f} deg'
            f'{", visualize" if self._visualize else ""}'
            f'{", saving frames" if self._save_dir else ""}')

    # --- rendering ---------------------------------------------------------
    def _project(self, pts_cam):
        """Nx3 camera-frame points -> Nx2 pixel coords."""
        ip, _ = cv2.projectPoints(np.asarray(pts_cam, dtype=np.float64),
                                  np.zeros(3), np.zeros(3), self._K, self._dist)
        return ip.reshape(-1, 2)

    def _make_background(self):
        """Grey canvas with a receding perspective floor grid (RGB, cosmetic)."""
        bg = np.full((self._H, self._W, 3), 105, np.uint8)
        if not self._grid_bg:
            return np.full((self._H, self._W, 3), 255, np.uint8)
        line = (88, 88, 88)
        y_floor = 0.5                      # floor 0.5 m below the camera (+y is down)
        xs = np.arange(-3.0, 3.001, 0.5)
        zs = np.arange(0.3, 6.001, 0.5)
        for x in xs:                        # lines of constant x, receding in z
            pl = self._project([[x, y_floor, z] for z in zs]).astype(np.int32)
            cv2.polylines(bg, [pl], False, line, 1, cv2.LINE_AA)
        for z in zs:                        # lines of constant z, spanning x
            pl = self._project([[x, y_floor, z] for x in xs]).astype(np.int32)
            cv2.polylines(bg, [pl], False, line, 1, cv2.LINE_AA)
        return bg

    def _render_marker(self, canvas, marker_id, R_cm, t_cm, px=240, pad=48):
        """Warp marker bitmap (+ white quiet zone) onto 3-channel canvas at (R_cm, t_cm)."""
        bits = cv2.aruco.generateImageMarker(self._dict, marker_id, px)
        padded = np.full((px + 2 * pad, px + 2 * pad), 255, np.uint8)
        padded[pad:pad + px, pad:pad + px] = bits
        # marker's true corners inside the padded bitmap (TL, TR, BR, BL)
        src = np.array([[pad, pad], [pad + px, pad],
                        [pad + px, pad + px], [pad, pad + px]], dtype=np.float32)
        dst = self._project((R_cm @ self._obj.T + t_cm.reshape(3, 1)).T).astype(np.float32)
        Hmg, _ = cv2.findHomography(src, dst)
        warped = cv2.warpPerspective(padded, Hmg, (self._W, self._H), borderValue=255)
        mask = cv2.warpPerspective(np.full_like(padded, 255), Hmg,
                                   (self._W, self._H), borderValue=0)
        canvas[mask > 0] = warped[mask > 0][:, None]   # broadcast grey -> 3 channels

    def _render_scene(self, scene):
        canvas = self._bg.copy()
        for mid, R_cm, t_cm in scene:
            self._render_marker(canvas, mid, R_cm, t_cm)
        return canvas                                   # RGB, HxWx3 uint8

    def _draw_overlay(self, rgb, scene):
        """BGR copy of the frame with ground-truth outline + pose axes + id labels."""
        vis = cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR)
        L = self._marker_m * 0.5
        for mid, R_cm, t_cm in scene:
            corners = self._project((R_cm @ self._obj.T + t_cm.reshape(3, 1)).T)
            cv2.polylines(vis, [corners.astype(np.int32)], True, (0, 255, 0), 2, cv2.LINE_AA)
            rvec, _ = cv2.Rodrigues(R_cm)
            cv2.drawFrameAxes(vis, self._K, self._dist, rvec, t_cm.reshape(3, 1), L, 2)
            org = tuple(corners[0].astype(int) + np.array([0, -8]))
            cv2.putText(vis, f'id {mid}  z={t_cm[2]:.2f}m', org,
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1, cv2.LINE_AA)
        cv2.putText(vis, 'ground-truth overlay (green=marker, RGB axes)', (8, self._H - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1, cv2.LINE_AA)
        return vis

    def _show_frame(self, rgb, scene):
        """Live window and/or PNG dump of the annotated frame (best-effort)."""
        if not (self._visualize or self._save_dir):
            return
        vis = self._draw_overlay(rgb, scene)
        if self._save_dir:
            cv2.imwrite(os.path.join(self._save_dir, f'frame_{self._frame_no:04d}.png'), vis)
        if self._visualize:
            try:
                cv2.imshow('aruco_sim', vis)
                cv2.waitKey(1)
            except cv2.error as e:                      # no display (headless)
                self.get_logger().warn(f'visualize disabled — no display ({e})')
                self._visualize = False
        self._frame_no += 1

    def _validate_scenes(self):
        """Self-check: every scene must render to markers cv2.aruco recovers at
        the generating pose. Guarantees the ground truth is correct."""
        for s, scene in enumerate(self._scenes):
            gray = cv2.cvtColor(self._render_scene(scene), cv2.COLOR_RGB2GRAY)
            corners, ids, _ = self._detector.detectMarkers(gray)
            found = set() if ids is None else set(int(i) for i in ids.flatten())
            for mid, R_cm, t_cm in scene:
                if mid not in found:
                    raise RuntimeError(
                        f'scene {s}: marker {mid} not self-detected — bad ground truth')
                idx = list(ids.flatten()).index(mid)
                c = corners[idx].reshape(-1, 2)
                ok, rvec, tvec = cv2.solvePnP(self._obj, c, self._K, self._dist,
                                              flags=cv2.SOLVEPNP_IPPE_SQUARE)
                t_err = np.linalg.norm(tvec.flatten() - t_cm) * 1000.0
                if not ok or t_err > 20.0:
                    raise RuntimeError(
                        f'scene {s}: marker {mid} self-pose off by {t_err:.1f} mm')
        self.get_logger().info('scene self-validation passed (ground truth OK)')

    # --- publish -----------------------------------------------------------
    def on_timer(self):
        if self._done:
            return
        # Wait for a detector to connect before counting frames — otherwise the
        # first scene is published into the void during discovery and counts as
        # a miss. With RELIABLE QoS, delivery is guaranteed once matched.
        if not self._started:
            if self._img_pub.get_subscription_count() < 1:
                return
            self._started = True
            self.get_logger().info('detector connected — starting scene sequence')
        scene = self._scenes[self._scene_idx]
        rgb = self._render_scene(scene)          # clean image published to the detector

        stamp = self.get_clock().now().to_msg()
        msg = Image()
        msg.header.stamp = stamp
        msg.header.frame_id = 'camera'
        msg.height = self._H
        msg.width = self._W
        msg.encoding = 'rgb8'
        msg.is_bigendian = 0
        msg.step = self._W * 3
        msg.data = rgb.tobytes()

        key = (stamp.sec, stamp.nanosec)
        self._gt[key] = {mid: (R, t) for mid, R, t in scene}
        self._n_expected += len(scene)
        self._img_pub.publish(msg)
        self._show_frame(rgb, scene)

        self._published += 1
        self._scene_idx += 1
        if self._scene_idx >= len(self._scenes):
            self._scene_idx = 0
            if self._cycles and self._published >= self._cycles * len(self._scenes):
                # give in-flight detections a moment, then summarise
                self._timer.cancel()
                self.create_timer(0.5, self._finish)

        # prune stale ground truth (detections should arrive within a frame or two)
        if len(self._gt) > 50:
            for k in sorted(self._gt)[:25]:
                del self._gt[k]

    # --- check -------------------------------------------------------------
    def on_detections(self, msg: ArucoDetections):
        key = (msg.header.stamp.sec, msg.header.stamp.nanosec)
        gt = self._gt.get(key)
        if gt is None:
            return  # detection for a frame we already pruned / not ours

        det = {}
        for i in range(msg.num_detections):
            mid = msg.marker_id[i]
            tvec = np.array(msg.tvec[i * 3:i * 3 + 3], dtype=np.float64)
            rvec = np.array(msg.rvec[i * 3:i * 3 + 3], dtype=np.float64)
            det[mid] = (tvec, rvec)

        for mid, (R_gt, t_gt) in gt.items():
            self._n_detected += (mid in det)
            if mid not in det:
                self.get_logger().warn(f'  marker {mid}: NOT DETECTED')
                continue
            self._n_id_ok += 1
            tvec, rvec = det[mid]
            t_err_mm = np.linalg.norm(tvec - t_gt) * 1000.0
            R_det = _rvec_to_R(rvec)
            # convention-robust orientation error: try R and its transpose
            e_direct = _geodesic_deg(R_det, R_gt)
            e_flip = _geodesic_deg(R_det.T, R_gt)
            o_err = min(e_direct, e_flip)
            if e_direct <= e_flip:
                self._conv_R += 1
            else:
                self._conv_Rt += 1

            self._tvec_errs.append(t_err_mm)
            self._orient_errs.append(o_err)
            t_ok = t_err_mm <= self._tvec_tol_mm
            o_ok = o_err <= self._orient_tol_deg
            self._n_tvec_ok += t_ok
            self._n_orient_ok += o_ok
            tag = 'PASS' if (t_ok and o_ok) else 'FAIL'
            self.get_logger().info(
                f'  [{tag}] id {mid:2d}: t_err={t_err_mm:6.1f} mm '
                f'(tol {self._tvec_tol_mm:.0f})  ang_err={o_err:5.2f} deg '
                f'(tol {self._orient_tol_deg:.0f})')

        del self._gt[key]

    # --- summary -----------------------------------------------------------
    def _finish(self):
        self._done = True
        n = max(self._n_id_ok, 1)
        tvec_arr = np.array(self._tvec_errs) if self._tvec_errs else np.array([0.0])
        orient_arr = np.array(self._orient_errs) if self._orient_errs else np.array([0.0])
        conv = ('R_marker_to_cam (matches contract)' if self._conv_R >= self._conv_Rt
                else 'TRANSPOSED rvec — convention flip vs detection_filter!')
        log = self.get_logger()
        log.info('================ aruco_sim summary ================')
        log.info(f'markers expected : {self._n_expected}')
        log.info(f'markers detected : {self._n_detected}  '
                 f'(miss {self._n_expected - self._n_detected})')
        log.info(f'tvec within tol  : {self._n_tvec_ok}/{n}   '
                 f'(mean {tvec_arr.mean():.1f} mm, max {tvec_arr.max():.1f} mm)')
        log.info(f'orient within tol: {self._n_orient_ok}/{n}   '
                 f'(mean {orient_arr.mean():.2f} deg, max {orient_arr.max():.2f} deg)')
        log.info(f'rvec convention  : {conv}')
        detect_frac = self._n_detected / max(self._n_expected, 1)
        all_correct = (self._n_tvec_ok == self._n_id_ok
                       and self._n_orient_ok == self._n_id_ok)
        overall = all_correct and detect_frac >= self._min_detect_frac
        log.info(f'detect fraction  : {detect_frac:.2f} (min {self._min_detect_frac:.2f})')
        log.info(f'OVERALL          : {"PASS" if overall else "FAIL"}')
        log.info('===================================================')
        raise SystemExit(0 if overall else 1)


def main(args=None):
    rclpy.init(args=args)
    node = ArucoSim()
    exit_code = 0
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except SystemExit as e:
        exit_code = int(e.code) if e.code is not None else 0
    finally:
        cv2.destroyAllWindows()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
    return exit_code


if __name__ == '__main__':
    main()
