import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch_ros.actions import Node, PushRosNamespace
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')

    return LaunchDescription([
        # Declare a launch argument
        DeclareLaunchArgument('loop', default_value='--loop', description='A loop parameter'),

        GroupAction([
            PushRosNamespace(rom_robot_namespace),
            # Pass the launch argument as a parameter
            Node(
                package='rom_waypoints_provider',
                executable='send_waypoints_goals_loop',
                name='send_waypoints_goals_loop',
                output='screen',
                arguments=[LaunchConfiguration('loop')]  
            )
        ])
    ])
