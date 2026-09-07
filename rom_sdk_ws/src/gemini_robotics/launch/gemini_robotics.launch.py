import os
from launch import LaunchDescription
from launch.actions import GroupAction
from launch_ros.actions import Node, PushRosNamespace


def generate_launch_description():
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')

    return LaunchDescription([
        GroupAction([
            PushRosNamespace(rom_robot_namespace),
            Node(
                package='gemini_robotics',
                executable='activate_gemini_robotics',
                name='gemini_robot_manager',
                output='screen',
            ),
        ])
    ])
