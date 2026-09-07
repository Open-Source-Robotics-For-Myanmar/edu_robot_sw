#!/usr/bin/env python3
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.event_handlers import OnProcessExit, OnProcessStart
from launch.actions import RegisterEventHandler,  TimerAction


def generate_launch_description():

    nav2_pkg = get_package_share_directory('edu_robot_nav2')

    switch_node = Node(
        name="switch",
        package="edu_robot_bringup",
        executable="switch_mode.py",
        output='screen',
    )

    tk_node = Node(
        package='edu_robot_bringup',
        executable='tk.py',
        name='tk',
        output='screen',
    )

    trigger_node = Node(
        package='edu_robot_bringup',
        executable='trigger',
        name='trigger_node',
        output='screen',
    )

    # Delay start of tk_node after `switch_node`
    delay_tk_node = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=switch_node,
            on_start=[tk_node],
        )
    )

     # Delay start of trigger_node after `tk_node`
    delay_trigger_node = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=switch_node,
            on_start=[trigger_node],
        )
    )
    delayed_trigger_time_node = TimerAction(
        period=7.0,  # Delay in seconds
        actions=[trigger_node]
    )

    return LaunchDescription(
        [
           switch_node,
            delay_tk_node,
           #delay_trigger_node,
           delayed_trigger_time_node,
        ]
    )


