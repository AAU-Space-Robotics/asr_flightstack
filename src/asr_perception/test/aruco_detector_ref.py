#!/usr/bin/env python3
"""
aruco_detector_ref — pure-OpenCV reference ArUco detector.

A drop-in stand-in for the MATLAB-Coder ``aruco_detector`` node: identical
topics, parameters and output convention, but it detects markers with
``cv2.aruco`` instead of generated code. Two uses:

  1. Exercise the ROS pipeline (and the ``aruco_sim`` test) WITHOUT MATLAB, so
     the plumbing can be validated before ``matlab/generate_code.m`` is run.
  2. Act as a ground-truth oracle to compare the MATLAB node against — both
     publish the same message on the same topic, so swapping executables in the
     launch file is the only change.

Subscribes: out/cam/synced/color        sensor_msgs/Image (rgb8/bgr8, 640x480)
Publishes:  aruco_detector/detections  asr_comms/ArucoDetections

Output convention (matches aruco_detector_node.cpp and the detection_filter
contract at detection_filter.cpp:476, where rodrigues(rvec) == R_marker_to_cam):
  tvec  = marker origin in the CAMERA frame, metres   (= solvePnP tvec)
  rvec  = Rodrigues vector of R_marker_to_cam          (= solvePnP rvec)

NOTE: cv_bridge is unusable in this environment (its compiled extension needs
NumPy < 2), so the raw Image buffer is converted with numpy directly.
"""

import numpy as np
import cv2

import rclpy
from rclpy.node import Node
from rclpy.qos import (QoSProfile, QoSReliabilityPolicy, QoSHistoryPolicy)

from sensor_msgs.msg import Image
from asr_comms.msg import ArucoDetections

MAX_MARKERS = 16  # mirror the codegen MAX_MARKERS cap in detect_aruco_in_camera_frame.m


def image_to_gray(msg: Image) -> np.ndarray:
    """Raw rgb8/bgr8 sensor_msgs/Image -> single-channel uint8 (cv_bridge-free)."""
    h, w = msg.height, msg.width
    buf = np.frombuffer(bytes(msg.data), dtype=np.uint8)
    # honour row stride (step) in case of padding, then drop it
    rgb = buf.reshape(h, msg.step)[:, : w * 3].reshape(h, w, 3)
    code = cv2.COLOR_RGB2GRAY if msg.encoding == 'rgb8' else cv2.COLOR_BGR2GRAY
    return cv2.cvtColor(rgb, code)


class ArucoDetectorRef(Node):
    def __init__(self):
        super().__init__('aruco_detector_ref')

        self.declare_parameter('fx',             425.88)
        self.declare_parameter('fy',             425.88)
        self.declare_parameter('cx',             430.51)
        self.declare_parameter('cy',             238.53)
        self.declare_parameter('marker_size_mm', 150.0)
        self.declare_parameter('dictionary',     'DICT_4X4_50')

        fx = self.get_parameter('fx').value
        fy = self.get_parameter('fy').value
        cx = self.get_parameter('cx').value
        cy = self.get_parameter('cy').value
        marker_m = self.get_parameter('marker_size_mm').value / 1000.0
        dict_name = self.get_parameter('dictionary').value

        self._K = np.array([[fx, 0.0, cx],
                            [0.0, fy, cy],
                            [0.0, 0.0, 1.0]], dtype=np.float64)
        self._dist = np.zeros((5, 1), dtype=np.float64)

        # Marker corner layout in object space (z=0 plane), matching
        # detect_aruco_in_camera_frame.m: TL, TR, BR, BL with +x right, +y up.
        half = marker_m / 2.0
        self._obj = np.array([[-half,  half, 0.0],
                              [ half,  half, 0.0],
                              [ half, -half, 0.0],
                              [-half, -half, 0.0]], dtype=np.float32)

        dictionary = cv2.aruco.getPredefinedDictionary(getattr(cv2.aruco, dict_name))
        self._detector = cv2.aruco.ArucoDetector(dictionary, cv2.aruco.DetectorParameters())

        # The real camera path is best_effort (see aruco_detector_node.cpp), but
        # best_effort drops most 921 kB frames over localhost UDP. As a test
        # detector we subscribe RELIABLE so no synthetic frame is lost; a
        # RELIABLE publisher is compatible with both this and the best_effort
        # MATLAB node.
        qos = QoSProfile(depth=5,
                         reliability=QoSReliabilityPolicy.RELIABLE,
                         history=QoSHistoryPolicy.KEEP_LAST)
        self._sub = self.create_subscription(
            Image, 'out/cam/synced/color', self.on_image, qos)
        self._pub = self.create_publisher(
            ArucoDetections, 'aruco_detector/detections', 10)

        self._warned_encoding = False
        self.get_logger().info(
            f'reference ArUco detector ready ({dict_name}, marker {marker_m*1000:.0f} mm)')

    def on_image(self, msg: Image):
        if msg.encoding not in ('rgb8', 'bgr8'):
            if not self._warned_encoding:
                self.get_logger().warn(
                    f"unexpected encoding '{msg.encoding}' — expected rgb8 or bgr8")
                self._warned_encoding = True
            return

        gray = image_to_gray(msg)
        corners, ids, _ = self._detector.detectMarkers(gray)
        if ids is None:
            return

        out = ArucoDetections()
        out.header = msg.header
        n = 0
        for c, mid in zip(corners, ids.flatten()):
            if n >= MAX_MARKERS:
                break
            ok, rvec, tvec = cv2.solvePnP(
                self._obj, c.reshape(-1, 2), self._K, self._dist,
                flags=cv2.SOLVEPNP_IPPE_SQUARE)
            if not ok:
                continue
            out.marker_id.append(int(mid))
            out.tvec.extend([float(tvec[0]), float(tvec[1]), float(tvec[2])])
            out.rvec.extend([float(rvec[0]), float(rvec[1]), float(rvec[2])])
            n += 1

        if n == 0:
            return
        out.num_detections = n
        self._pub.publish(out)


def main(args=None):
    rclpy.init(args=args)
    node = ArucoDetectorRef()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
