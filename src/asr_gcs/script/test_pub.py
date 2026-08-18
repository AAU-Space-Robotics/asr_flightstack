#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from asr_comms.msg import ProbeLocations
from std_msgs.msg import Header
import random
import time


class ProbeTestPublisher(Node):
    def __init__(self):
        super().__init__('probe_test_publisher')

        self.probe_pub = self.create_publisher(ProbeLocations, 'probe/probe_locations', 10)

        self.start_time = time.time()
        self.timer = self.create_timer(1.0, self.publish_probes)  # 1 Hz

        # Base position each probe drifts around, so they look like plausible detections
        self.base_x = 20.0
        self.base_y = -3.0
        self.base_z = -4.0

    def publish_probes(self):
        elapsed = time.time() - self.start_time

        # Cycle probe count 1 -> 5 -> 1 every 10 seconds, so you can watch the panel grow/shrink
        cycle = int(elapsed) % 10
        num_probes = (cycle % 5) + 1

        msg = ProbeLocations()
        msg.header = Header()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "map"
        msg.num_probes = num_probes

        positions = []
        uncertainty = []

        for i in range(num_probes):
            # Spread probes out a bit so they're visually distinct
            x = self.base_x + i * 1.5 + random.uniform(-0.05, 0.05)
            y = self.base_y + i * 1.0 + random.uniform(-0.05, 0.05)
            z = self.base_z + random.uniform(-0.02, 0.02)

            positions.extend([x, y, z])

            sigma_x = random.uniform(0.01, 0.1)
            sigma_y = random.uniform(0.01, 0.1)
            sigma_z = random.uniform(0.01, 0.1)
            uncertainty.extend([sigma_x, sigma_y, sigma_z])

        msg.positions = positions
        msg.uncertainty = uncertainty

        self.probe_pub.publish(msg)
        self.get_logger().info(f"Published {num_probes} probes")


def main():
    rclpy.init()
    node = ProbeTestPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()