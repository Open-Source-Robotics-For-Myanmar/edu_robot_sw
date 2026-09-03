#!/usr/bin/env python3
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
import xacro


from std_msgs.msg import String

launch = None
uuid = None

def generate_launch_description():

    
    global  m , uuid


    description_pkg = get_package_share_directory('edu_robot_bringup')
    
    mapping_mode = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
        description_pkg,'launch','mapping.launch.py'
        )])
    )
    nav_mode = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
        description_pkg,'launch','nav2_mode.launch.py'
        )])
    )
    remapping_mode = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
        description_pkg,'launch','remapping.launch.py'
        )])
    )

    # IncludeLaunchDescription(
    #         os.path.join(description_pkg,"launch","mapping.launch.py")
    #         condition=IfCondition(LaunchConfiguration('mode').equals('mode1'))
    #     ),

    # IncludeLaunchDescription(
    #         'path_to_mode2_launch_file.launch.py',
    #         condition=IfCondition(LaunchConfiguration('mode').equals('mode2'))
    #     ),

    # IncludeLaunchDescription(
    #         'path_to_mode3_launch_file.launch.py',
    #         condition=IfCondition(LaunchConfiguration('mode').equals('mode3'))
    #     ),

    

    return LaunchDescription(
        [
            DeclareLaunchArgument('open_rviz', default_value='true', description='Open RViz.'),
            mapping_mode,
            nav_mode,
            remapping_mode,
        ]
    )