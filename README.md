# Nav2 Multi-Mode Navigation

A ROS 2 Jazzy package set for switching between three navigation modes while reusing a single Nav2 navigation stack.

## Target platform

- Ubuntu 24.04
- ROS 2 Jazzy
- Nav2 (Jazzy)
- C++ / BehaviorTree.CPP plugins

## Navigation modes

1. **Free navigation** — use the standard Nav2 `NavigateToPose` behavior tree to navigate to a goal pose.
2. **Route navigation** — use the Nav2 Route Server and the Jazzy route-graph behavior tree to navigate along a configured route graph.
3. **External path following** — obtain a `nav_msgs/msg/Path` from a dedicated topic via a custom BT action, validate that it belongs to the active mission and is fresh, then execute it with Nav2's `FollowPath` action.

## Design principles

- Reuse one Nav2 planner/controller/costmap/smoother/behavior-server stack.
- Select the navigation mode through the `behavior_tree` field of a `NavigateToPose` goal, or through a thin mission API that maps a mode to a known BT XML.
- Do not treat `NavigateToPose` as a transport for arbitrary paths; external paths use a separate topic and are consumed by the custom BT node.
- Handle mission identity, path freshness, cancellation, and mode switching explicitly.
- Keep custom BT nodes small and testable; avoid duplicating Nav2 servers.

## Planned package layout

```text
nav2_multi_mode_navigation/
├── README.md
├── nav2_multi_mode_navigation/
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── include/nav2_multi_mode_navigation/
│   ├── src/
│   ├── behavior_trees/
│   ├── config/
│   ├── launch/
│   └── test/
└── docs/
```

## Implementation status

This repository is being scaffolded. The architecture and package interfaces must be verified against the installed ROS 2 Jazzy/Nav2 versions before claiming deployment readiness. Hardware-specific frames, topics, route graph data, and controller parameters remain deployment configuration.

## Initial development sequence

1. Confirm Jazzy/Nav2 dependencies and package interfaces.
2. Add the package metadata, build system, and baseline configuration.
3. Add the external-path BT action with timeout/freshness/mission checks.
4. Add and validate the three behavior trees.
5. Add launch files and tests for cancellation and mode switching.
6. Run `colcon build` and integration tests on Ubuntu 24.04 + ROS 2 Jazzy.
