#!/usr/bin/env python3
"""Publish a sample MissionPath repeatedly for external-path smoke tests.

Example: publish_mission_path.py --mission-id demo-001 --points '0,0;1,0;2,1'
"""
import argparse
import time

import rclpy
from geometry_msgs.msg import PoseStamped
from nav2_multi_mode_navigation.msg import MissionPath
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, HistoryPolicy, QoSProfile, ReliabilityPolicy
from std_msgs.msg import String


class MissionPathPublisher(Node):
    def __init__(self, mission_id, points, frame, duration, rate):
        super().__init__("mission_path_demo_publisher")
        active_qos = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        path_qos = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.VOLATILE,
        )
        self.active_pub = self.create_publisher(String, "/active_mission_id", active_qos)
        self.path_pub = self.create_publisher(MissionPath, "/mission_path", path_qos)
        active = String()
        active.data = mission_id
        self.active_pub.publish(active)
        self.message = MissionPath()
        self.message.mission_id = mission_id
        self.message.path.header.frame_id = frame
        for x, y in points:
            pose = PoseStamped()
            pose.header.frame_id = frame
            pose.pose.position.x = x
            pose.pose.position.y = y
            pose.pose.orientation.w = 1.0
            self.message.path.poses.append(pose)
        self.deadline = time.monotonic() + duration
        self.timer = self.create_timer(1.0 / rate, self.publish_path)
        self.get_logger().info(
            "Publishing mission '%s' with %d poses on /mission_path for %.1f seconds" %
            (mission_id, len(points), duration))

    def publish_path(self):
        if time.monotonic() >= self.deadline:
            rclpy.shutdown()
            return
        now = self.get_clock().now().to_msg()
        self.message.path.header.stamp = now
        for pose in self.message.path.poses:
            pose.header.stamp = now
        self.path_pub.publish(self.message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mission-id", required=True)
    parser.add_argument("--points", required=True,
                        help="Semicolon-separated x,y pairs, e.g. '0,0;1,0;2,1'")
    parser.add_argument("--frame", default="map")
    parser.add_argument("--duration", type=float, default=10.0)
    parser.add_argument("--rate", type=float, default=2.0)
    args, ros_args = parser.parse_known_args()
    if args.duration <= 0 or args.rate <= 0:
        parser.error("--duration and --rate must be positive")
    try:
        points = []
        for item in args.points.split(";"):
            x, y = item.split(",", maxsplit=1)
            points.append((float(x), float(y)))
        if not points:
            raise ValueError("at least one x,y point is required")
    except ValueError as exc:
        parser.error("invalid --points value: %s" % exc)

    rclpy.init(args=ros_args)
    node = MissionPathPublisher(args.mission_id, points, args.frame, args.duration, args.rate)
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if rclpy.ok():
            rclpy.shutdown()
        node.destroy_node()


if __name__ == "__main__":
    main()
