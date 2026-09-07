from launch import LaunchDescription
from ament_index_python.packages import get_package_share_directory
import launch_ros.actions
import os
import yaml
from launch.substitutions import EnvironmentVariable
import pathlib
import launch.actions
from launch.actions import DeclareLaunchArgument

def generate_launch_description():
    rom_robot_name = os.environ.get('ROM_ROBOT_MODEL', 'edu_robot')
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
    return LaunchDescription([
        launch_ros.actions.Node(
            package='robot_localization',
            executable='ekf_node',
            name='ekf_filter_node',
            namespace=rom_robot_namespace,
            output='screen',
            parameters=[os.path.join(get_package_share_directory(f'{rom_robot_name}_ekf'), 'config', 'ekf.yaml')],
            remappings=[('cmd_vel', 'diff_controller/cmd_vel_unstamped'),('odometry/filtered' , 'odom' ), ('set_pose' , 'initialpose')],
           ),
        launch_ros.actions.Node(
            package='rom_waypoints_provider',
            executable='robot_pose_publisher',
            name='robot_pose_publisher',
            namespace=rom_robot_namespace,
           ),

])
