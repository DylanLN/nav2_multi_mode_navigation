# Nav2 Multi-Mode Navigation

ROS 2 Jazzy / Ubuntu 24.04 package for integrating three navigation modes into one Nav2 stack:

- **Free navigation** through a selectable Nav2 behavior tree.
- **Route-graph navigation** using the Jazzy Route Server's `ComputeAndTrackRoute` BT action.
- **External path following** through the custom `GetPathFromTopic` BT plugin, with mission-ID matching and path freshness checks.

The ROS 2 package is in [`nav2_multi_mode_navigation/`](nav2_multi_mode_navigation/). It includes the custom `MissionPath` message, behavior trees, BT plugin registration, a Nav2 bringup wrapper, a mode-selecting `NavigateToPose` client, and an external-path test publisher.

See the [package README](nav2_multi_mode_navigation/README.md) for build instructions, parameter integration, launch commands, test procedures, and known limitations.

**Important:** `config/bt_navigator_plugins.yaml` is a plugin-list fragment, not a complete Nav2 parameter file. Merge its entries into your robot's existing BT Navigator configuration and preserve standard Nav2 plugins. The launch wrapper expects a complete robot/simulator-specific Nav2 params file, map, TF, odometry and sensor configuration.
