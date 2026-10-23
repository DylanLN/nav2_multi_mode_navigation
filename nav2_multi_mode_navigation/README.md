# nav2_multi_mode_navigation (ROS 2 Jazzy)

Initial implementation scaffold for three Nav2 navigation modes:
- free navigation via a standard Nav2 BT;
- route-graph navigation using Nav2 Route Server / its Jazzy BT nodes;
- externally generated path following using a custom BT action.

## Important interface choice

A plain `nav_msgs/msg/Path` has no mission identifier. This package defines `MissionPath` (`mission_id` + `nav_msgs/Path`) so the BT node can reject paths belonging to another mission. Publish fresh, non-empty paths with a non-zero `path.header.stamp`, in a frame configured consistently with Nav2 (normally `map`).

## Build

```bash
cd ~/your_ros2_ws/src
git clone https://github.com/DylanLN/nav2_multi_mode_navigation.git
cd ..
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-up-to nav2_multi_mode_navigation
source install/setup.bash
```

## Configure

Merge `config/bt_navigator_plugins.yaml` into your existing Nav2 parameter file. Keep the other plugin names already required by your installed Nav2 configuration. Configure the selected BT XML through the `behavior_tree` field of a `NavigateToPose` goal.

For the external-path tree, the caller must set the BT blackboard entry `mission_id` before the tree runs. The input path topic is `/mission_path`, type `nav2_multi_mode_navigation/msg/MissionPath`.

## Publish a test message

Once built and sourced, publish a message using a small ROS 2 publisher that fills:
- `mission_id`: exactly the mission ID expected by the BT;
- `path.header.frame_id`: a frame accepted by your Nav2 controller (typically `map`);
- `path.header.stamp`: current ROS time;
- `path.poses`: at least one valid pose.

## Caveats for first validation

- This scaffold has not been compiled against your machine's installed Jazzy packages yet.
- The minimal free-navigation BT is a bring-up example, not a replacement for Nav2's full default tree with replanning and recovery.
- Route graph navigation is intentionally left to the installed Jazzy Route Server BT and graph configuration; its exact XML should be copied from the installed Nav2 version before wiring it into a production mission dispatcher.
- Verify that the installed Nav2 blackboard exposes `node` as `rclcpp::Node::SharedPtr`. If your installed Jazzy headers use a lifecycle-node pointer type, adjust the type to match.
- The BT node waits asynchronously across BT ticks; the executor must continue processing the BT Navigator node's subscription callbacks.

## Suggested first checks

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select nav2_multi_mode_navigation --event-handlers console_direct+
```

If compilation fails, send the complete first compiler error and the output of:
```bash
ros2 pkg prefix nav2_behavior_tree
grep -R 'blackboard->get.*node' /opt/ros/jazzy/include/nav2_behavior_tree
```
