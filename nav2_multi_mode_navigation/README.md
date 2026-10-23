# nav2_multi_mode_navigation

ROS 2 Jazzy / Ubuntu 24.04 package demonstrating three navigation modes on one Nav2 stack.

## Included

- `behavior_trees/navigate_to_pose.xml`: minimal free-navigation tree using `ComputePathToPose` and `FollowPath`.
- `behavior_trees/navigate_on_route_graph.xml`: route-graph tree using Jazzy's `ComputeAndTrackRoute` action.
- `behavior_trees/navigate_with_external_path.xml`: receives a mission-scoped path through the custom `GetPathFromTopic` BT action and passes it to Nav2 `FollowPath`.
- `msg/MissionPath.msg`: a `mission_id` plus `nav_msgs/Path`.
- `launch/multi_mode_navigation.launch.py`: wrapper around the standard `nav2_bringup/bringup_launch.py`.
- `scripts/send_navigation_goal.py`: sends a `NavigateToPose` goal with the chosen behavior-tree XML.
- `scripts/publish_mission_path.py`: repeatedly publishes a sample path for external-mode smoke tests.
- `config/bt_navigator_plugins.yaml`: plugin-library overlay fragment, not a complete Nav2 parameter file.

## Build

```bash
cd ~/code/nav2_ws
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to nav2_multi_mode_navigation --event-handlers console_direct+
source install/setup.bash
```

Verify the interface and installed files:

```bash
ros2 interface show nav2_multi_mode_navigation/msg/MissionPath
ros2 pkg prefix nav2_multi_mode_navigation
ros2 pkg executables nav2_multi_mode_navigation
```

## Configure the BT Navigator

Merge the two entries in `config/bt_navigator_plugins.yaml` into the existing `bt_navigator.ros__parameters.plugin_lib_names` list in your robot's **complete** Nav2 parameter YAML. Keep all standard Nav2 plugin libraries already in the list.

Required additional libraries:

- `nav2_multi_mode_navigation_bt_node`
- `nav2_compute_and_track_route_bt_node` (only needed for route-graph mode)

Do not pass `bt_navigator_plugins.yaml` directly as the full Nav2 parameter file. It contains only a fragment. See `config/README.md`.

## Launch Nav2

Use a valid map and a complete Nav2 parameter file for your robot or simulator:

```bash
ros2 launch nav2_multi_mode_navigation multi_mode_navigation.launch.py \
  map:=/absolute/path/to/map.yaml \
  params_file:=/absolute/path/to/complete_nav2_params.yaml \
  use_sim_time:=false
```

This launch file brings up the standard Nav2 stack. It does not invent a robot-specific map, controller configuration, TF tree, sensor setup, or route graph. Ensure localization, TF, odometry, costmaps, controller IDs and goal-checker IDs match your platform.

## Select a navigation mode

The example goal client chooses a BT XML by setting `NavigateToPose.Goal.behavior_tree`. Run after Nav2 is active:

```bash
# Free navigation
ros2 run nav2_multi_mode_navigation send_navigation_goal.py --mode free --x 2.0 --y 1.0 --yaw 0.0

# Route-graph navigation (Route Server and graph must be configured and active)
ros2 run nav2_multi_mode_navigation send_navigation_goal.py --mode route --x 2.0 --y 1.0

# External path following (publish a matching MissionPath at the same time)
ros2 run nav2_multi_mode_navigation send_navigation_goal.py --mode external --x 2.0 --y 1.0
```

Coordinates are in the selected frame (default `map`). The free-navigation tree is intentionally minimal for bring-up; it does not include the full default Nav2 recovery and replanning behavior tree. Use the installed Jazzy default tree as the basis for production deployments.

## External-path protocol

1. Publish `/active_mission_id` as `std_msgs/msg/String`, using reliable + transient-local QoS.
2. Publish `/mission_path` as `nav2_multi_mode_navigation/msg/MissionPath`, using the same `mission_id`. The path publisher can use reliable + volatile QoS.
3. Set `path.header.frame_id` to a frame Nav2 can transform (normally `map`), provide at least one pose, and stamp the path with the current ROS clock.
4. The BT node waits up to `timeout_ms` (default 5000), rejects empty or stale paths, and requires the path mission ID to match the active ID. The default maximum age is `max_age_ms=3000`.

For a smoke test, start an external-mode goal and, in another terminal, publish a short path repeatedly:

```bash
ros2 run nav2_multi_mode_navigation publish_mission_path.py \
  --mission-id demo-001 --points '0,0;1,0;2,1' --frame map --duration 10
```

Replace these sample points with safe coordinates inside your test map. The test publisher is a diagnostic helper, not a production path planner. A mission dispatcher should coordinate the active ID, path publication, cancellation, and mission transitions.

## Route-graph requirements

Route mode needs the Jazzy Route Server configured with a valid graph and active lifecycle state. Loading the BT library alone is not sufficient. Check the installed Jazzy Route Server parameters and graph schema for your exact Nav2 version, and verify the route graph covers the requested start/goal. Route mode is separate from the regular `FollowPath` pipeline.

## Current limitations

- This is an integration scaffold, not a complete robot-specific bring-up. A map, localization, TF, odometry, sensors and controller configuration must come from your robot or simulation.
- The free-navigation BT is a minimal example without the full Nav2 recovery/replanning tree.
- The external-path demo publisher does not calculate collision-free paths; it only helps verify message transport and BT wiring.
- Route Server graph configuration is deployment-specific and is not auto-generated by this package.
- Compilation success verifies build/install, not runtime correctness on every Nav2 Jazzy patch release. Validate each mode in simulation before using a physical robot.
