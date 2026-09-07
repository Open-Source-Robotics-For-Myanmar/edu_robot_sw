import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    """Launch ros_tcp_endpoint for Unity AR Robot App connection."""

    ros_ip = LaunchConfiguration('ros_ip', default='0.0.0.0')
    ros_tcp_port = LaunchConfiguration('ros_tcp_port', default='10000')

    return LaunchDescription([
        DeclareLaunchArgument(
            'ros_ip',
            default_value='0.0.0.0',
            description='IP address to bind the TCP endpoint'
        ),
        DeclareLaunchArgument(
            'ros_tcp_port',
            default_value='10000',
            description='Port for TCP endpoint'
        ),
        Node(
            package='ros_tcp_endpoint',
            executable='default_server_endpoint',
            name='ros_tcp_endpoint',
            parameters=[{
                'ROS_IP': ros_ip,
                'ROS_TCP_PORT': ros_tcp_port,
            }],
            output='screen'
        ),
    ])
