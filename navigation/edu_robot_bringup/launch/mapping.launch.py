from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='edu_robot_bringup',
            executable='mapping',
            name='mapping_mode',
            output='screen'
        )
    ])
