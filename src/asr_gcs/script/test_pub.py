#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from asr_comms.msg import TelemetryGPS, TelemetryOriginGPS, BaseStationStats
import math
import time


class TestPublisher(Node):
    def __init__(self):
        super().__init__('test_telemetry_publisher')

        self.gps_pub = self.create_publisher(TelemetryGPS, 'telemetry/gps', 10)
        self.origin_gps_pub = self.create_publisher(TelemetryOriginGPS, 'telemetry/origin_gps', 10)
        self.base_station_pub = self.create_publisher(BaseStationStats, 'basestation_stats', 10)

        self.start_time = time.time()
        self.timer = self.create_timer(0.2, self.publish_all)  # 5 Hz

    def publish_all(self):
        now = time.time()
        elapsed = now - self.start_time

        base_lat = 57.014595
        base_lon = 9.986395

        # --- GPS ---
        gps_msg = TelemetryGPS()
        gps_msg.timestamp = now
        gps_msg.latitude = base_lat + 0.0001 * math.sin(elapsed * 0.1)
        gps_msg.longitude = base_lon + 0.0001 * math.cos(elapsed * 0.1)
        gps_msg.satellites_used = 12
        self.gps_pub.publish(gps_msg)

        # --- Origin GPS ---
        origin_msg = TelemetryOriginGPS()
        origin_msg.timestamp = now
        origin_msg.latitude = base_lat
        origin_msg.longitude = base_lon
        origin_msg.altitude = 42.0
        self.origin_gps_pub.publish(origin_msg)

        # --- Base Station Stats ---
        bs_msg = BaseStationStats()
        bs_msg.basestation_connected = True

        # Cycle through survey phases for testing: INACTIVE -> SURVEY_IN -> STREAMING
        cycle = elapsed % 30.0
        if cycle < 5.0:
            bs_msg.rtk_status = 0  # RTK_STATUS_INACTIVE
        elif cycle < 20.0:
            bs_msg.rtk_status = 1  # RTK_STATUS_SURVEY_IN
        else:
            bs_msg.rtk_status = 2  # RTK_STATUS_STREAMING

        bs_msg.rtk_accuracy_target = 0.02
        # Simulate accuracy improving over time, then continuing to improve slightly past target
        progress = min(cycle / 20.0, 1.5)
        bs_msg.rtk_accuracy = max(0.005, 2.0 * (1.0 - progress / 1.5))
        bs_msg.rtk_survey_duration = min(cycle, 25.0)
        bs_msg.rtk_survey_duration_max = 20.0

        self.base_station_pub.publish(bs_msg)


def main():
    rclpy.init()
    node = TestPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()