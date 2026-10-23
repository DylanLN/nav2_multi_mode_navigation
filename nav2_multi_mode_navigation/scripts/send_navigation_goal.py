#!/usr/bin/env python3
"""Send NavigateToPose with one of this package's behavior trees."""
import argparse
import math
import os

import rclpy
from ament_index_python.packages import get_package_share_directory
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import NavigateToPose
from rclpy.action import ActionClient
from rclpy.node import Node


class NavigationGoalClient(Node):
    def __init__(self):
        super().__init__("multi_mode_navigation_goal_client")
        self.client = ActionClient(self, NavigateToPose, "navigate_to_pose")

    def send(self, x, y, yaw, frame, behavior_tree):
        self.get_logger().info("Waiting for navigate_to_pose action server...")
        if not self.client.wait_for_server(timeout_sec=20.0):
            raise RuntimeError("navigate_to_pose action server was not available")

        goal = NavigateToPose.Goal()
        goal.pose = PoseStamped()
        goal.pose.header.frame_id = frame
        goal.pose.header.stamp = self.get_clock().now().to_msg()
        goal.pose.pose.position.x = x
        goal.pose.pose.position.y = y
        goal.pose.pose.orientation.z = math.sin(yaw / 2.0)
        goal.pose.pose.orientation.w = math.cos(yaw / 2.0)
        goal.behavior_tree = behavior_tree

        future = self.client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self, future)
        handle = future.result()
        if handle is None or not handle.accepted:
            raise RuntimeError("Navigation goal was rejected")
        self.get_logger().info("Goal accepted; waiting for result")
        result_future = handle.get_result_async()
        rclpy.spin_until_future_complete(self, result_future)
        result = result_future.result()
        if result is None:
            raise RuntimeError("Navigation result was not returned")
        self.get_logger().info("Navigation action finished with status %s" % result.status)
        return result.status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("free", "route", "external"), required=True)
    parser.add_argument("--x", type=float, required=True)
    parser.add_argument("--y", type=float, required=True)
    parser.add_argument("--yaw", type=float, default=0.0, help="Goal yaw in radians")
    parser.add_argument("--frame", default="map")
    args, ros_args = parser.parse_known_args()

    rclpy.init(args=ros_args)
    node = NavigationGoalClient()
    try:
        share = get_package_share_directory("nav2_multi_mode_navigation")
        tree_names = {
            "free": "navigate_to_pose.xml",
            "route": "navigate_on_route_graph.xml",
            "external": "navigate_with_external_path.xml",
        }
        tree = os.path.join(share, "behavior_trees", tree_names[args.mode])
        node.send(args.x, args.y, args.yaw, args.frame, tree)
    except Exception as exc:  # Report action and setup failures as a useful CLI error.
        node.get_logger().error(str(exc))
        raise
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
