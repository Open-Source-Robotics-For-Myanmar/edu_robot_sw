from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='edu_robot_bringup',
            executable='nav2',
            name='nav_mode',
            output='screen'
        )
    ])
