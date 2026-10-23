#!/usr/bin/env python3
"""Launch the standard Nav2 stack with a user-supplied complete parameter file.

The supplied params file must include the normal Nav2 parameters plus this
package's BT plugin libraries. This wrapper intentionally does not pretend that
a plugin-only YAML fragment is a complete Nav2 configuration.
"""
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    map_arg = DeclareLaunchArgument(
        "map", description="Absolute path to the occupancy-grid map YAML")
    params_arg = DeclareLaunchArgument(
        "params_file", description="Absolute path to a complete Nav2 params YAML")
    use_sim_time_arg = DeclareLaunchArgument("use_sim_time", default_value="false")
    autostart_arg = DeclareLaunchArgument("autostart", default_value="true")
    namespace_arg = DeclareLaunchArgument("namespace", default_value="")
    use_respawn_arg = DeclareLaunchArgument("use_respawn", default_value="false")

    bringup = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare("nav2_bringup"), "launch", "bringup_launch.py"])),
        launch_arguments={
            "map": LaunchConfiguration("map"),
            "params_file": LaunchConfiguration("params_file"),
            "use_sim_time": LaunchConfiguration("use_sim_time"),
            "autostart": LaunchConfiguration("autostart"),
            "namespace": LaunchConfiguration("namespace"),
            "use_respawn": LaunchConfiguration("use_respawn"),
        }.items(),
    )

    return LaunchDescription([
        map_arg, params_arg, use_sim_time_arg, autostart_arg,
        namespace_arg, use_respawn_arg, bringup,
    ])
