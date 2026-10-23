# Nav2 configuration notes

`bt_navigator_plugins.yaml` is an overlay fragment, not a complete Nav2 parameter file. Merge its entries into the `bt_navigator.ros__parameters.plugin_lib_names` list in the full parameter YAML used by your robot/simulator. Preserve the standard Nav2 BT libraries already present in that list.

For the three modes, the BT navigator must be able to load:

- `nav2_multi_mode_navigation_bt_node` for the custom `GetPathFromTopic` action.
- `nav2_compute_and_track_route_bt_node` for route-graph mode (and the Route Server must also be configured and active).

The plugin library names above are additional entries; do not replace your existing list with only these two names. Also make sure your selected controller and goal-checker IDs match the IDs defined by your Nav2 configuration. The sample BT XML uses `FollowPath` and `general_goal_checker`.

The launch file accepts a complete Nav2 params file from the caller. It intentionally does not pass this small overlay as if it were a complete configuration.
