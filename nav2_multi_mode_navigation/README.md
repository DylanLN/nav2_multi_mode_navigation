# nav2_multi_mode_navigation (ROS 2 Jazzy)

Initial implementation for three Nav2 navigation modes:
- **Free navigation:** `behavior_trees/navigate_to_pose.xml` (minimal bring-up tree).
- **Route-graph navigation:** `behavior_trees/navigate_on_route_graph.xml` using the Jazzy `ComputeAndTrackRoute` BT action.
- **External path following:** `behavior_trees/navigate_with_external_path.xml` using a custom BT action and Nav2's `FollowPath`.

## Important interface choice

A plain `nav_msgs/msg/Path` has no mission identifier. This package defines `MissionPath` (`mission_id` + `nav_msgs/Path`) so the BT node can reject paths belonging to another mission. Publish fresh, non-empty paths with a non-zero `path.header.stamp`, in a frame configured consistently with Nav2 (normally `map`).

For external-path mode, the mission dispatcher publishes the active ID on `/active_mission_id` as `std_msgs/msg/String` using **reliable + transient-local QoS**. It then publishes a `MissionPath` on `/mission_path` with the same ID. The BT node only passes a matching, fresh path to `FollowPath`. This avoids relying on a custom blackboard entry that a standard `NavigateToPose` goal cannot set.

## Build on Ubuntu 24.04 / ROS 2 Jazzy

```bash
cd ~/your_ros2_ws/src
git clone https://github.com/DylanLN/nav2_multi_mode_navigation.git
cd ..
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to nav2_multi_mode_navigation
source install/setup.bash
```

## Configure Nav2

1. Merge the `plugin_lib_names` entries from `config/bt_navigator_plugins.yaml` into your existing `bt_navigator.ros__parameters.plugin_lib_names` list. **Do not replace the full existing list** with this small fragment.
2. For route-graph navigation, ensure `nav2_compute_and_track_route_bt_node` is also in that list, and configure/activate the Route Server with a valid route graph.
3. Select the desired XML using the `behavior_tree` field of the `NavigateToPose` goal.

The free-navigation XML here is deliberately minimal for initial bring-up; use the installed Jazzy default tree for production recovery/replanning behavior.

## First validation

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select nav2_multi_mode_navigation --event-handlers console_direct+
```

This repository has not yet been compiled on your target machine. The first compile is the source of truth for your exact installed Jazzy patch level. In particular, if the compiler reports a type mismatch for the BT blackboard `node` key, compare it with the installed Nav2 selector plugin headers/sources; the current implementation expects `rclcpp::Node::SharedPtr`.

For route navigation, the BT action runs the Route Server's route-tracking action; it is not interchangeable with the standard `FollowPath` action. For external paths, the custom node waits asynchronously across BT ticks, checks the mission ID and path timestamp, and fails on timeout or stale/empty input.
