#!/usr/bin/env python3
"""
ROM Robotics - Dabai 3D Camera Launch (Namespace-aware)

Reads ROM_ROBOT_NAMESPACE env var (default: 'default_robot1') and wraps
all camera nodes, TF publishers, PCL merge, and PointCloud-to-LaserScan
inside GroupAction + PushRosNamespace.

Topics will appear under /<namespace>/... e.g. /default_robot1/camera/scan
"""
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, GroupAction
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node, PushRosNamespace
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'edu_robot')
    rom_robot_model = os.environ.get('ROM_ROBOT_MODEL', 'edu_robot')

    pkg_share = get_package_share_directory('rom_dabai_3d_camera')

    # Launch arguments
    use_rviz = LaunchConfiguration('use_rviz')
    use_usb_cam = LaunchConfiguration('use_usb_cam')
    pi_5 = LaunchConfiguration('pi_5')
    pi_4 = LaunchConfiguration('pi_4')
    device_1_id = LaunchConfiguration('device_1_id')
    device_2_id = LaunchConfiguration('device_2_id')
    # device_3_id = LaunchConfiguration('device_3_id')

    # ───── Multi camera nodes (XML include) ─────
    multi_camera_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_share, 'launch', 'multi_camera.launch.py')
        ),
        launch_arguments={
            'camera1_serial_number': device_1_id,
            'camera2_serial_number': device_2_id,
            # 'camera3_serial_number': device_3_id,
        }.items()
    )

    # ───── PointCloud to LaserScan (relative topics for namespace support) ─────
    p2l_node = Node(
        package='pointcloud_to_laserscan',
        executable='pointcloud_to_laserscan_node',
        name='pointcloud_to_laserscan',
        output='screen',
        remappings=[
            ('cloud_in', 'filter/voxel'),
            ('scan', 'camera/scan'),
        ],
        parameters=[{
            'target_frame': 'base_link',
            'transform_tolerance': 0.01,
            'min_height': 0.0,
            'max_height': 1.2,
            'angle_min': -1.1,
            'angle_max': 1.1,
            'angle_increment': 0.00611,
            'scan_time': 0.03333,
            'range_min': 0.15,
            'range_max': 4.3,
            'use_inf': True,
            'inf_epsilon': 1.0,
            'concurrency_level': 1,
        }]
    )

    # ───── PCL Merge node ─────
    pcl_merge_node = Node(
        package='rom_pcl_filters',
        executable='rom_pcl_merge',
        name='rom_pcl_merge1',
        output='screen',
        parameters=[{
            'height': 0.15,
            'model': rom_robot_model,
            'multi_robot_name': rom_robot_namespace,
        }]
    )

    # ───── Static TF: base_link → camera1_link ─────
    camera_tf_1 = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='camera_broadcaster_01',
        output='screen',
        arguments=[
            '0.220', '0.00', '1.110',
            '0.0', '0.78', '0.0',
            'base_link', 'camera_top_link',
        ]
    )

    # ───── Static TF: base_link → camera2_link ─────
    camera_tf_2 = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='camera_broadcaster_02',
        output='screen',
        arguments=[
            '0.220', '0.00', '0.160',
            '0.0', '-0.78', '0.0',
            'base_link', 'camera_bottom_link',
        ]
    )

    # ───── RViz2 (optional) ─────
    rviz_config = os.path.join(
        get_package_share_directory('rom_pcl_filters'), 'rviz2', 'merge.rviz'
    )
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config],
        condition=IfCondition(use_rviz),
    )
    usb_cam_config = os.path.join(
        get_package_share_directory('rom_dabai_3d_camera'), 'config', 'usb_cam.yaml'
    )
    usb_cam = Node(
        package='usb_cam',
        executable='usb_cam_node_exe',
        name='usb_cam',
        output='screen',
        arguments=['-d', usb_cam_config],
        condition=IfCondition(use_usb_cam),
    )

    # ───── Wrap ALL nodes under ROM_ROBOT_NAMESPACE ─────
    namespaced_nodes = GroupAction(
        actions=[
            PushRosNamespace(rom_robot_namespace),
            multi_camera_launch,
            p2l_node,
            pcl_merge_node,
            camera_tf_1,
            camera_tf_2,
            rviz_node,
            # usb_cam,
        ]
    )

    return LaunchDescription([
        DeclareLaunchArgument('use_rviz', default_value='true', description='Launch RViz2'),
        DeclareLaunchArgument('use_usb_cam', default_value='true', description='Launch USB Cam'),
        DeclareLaunchArgument('pi_5', default_value='-0.6', description='Camera1 pitch'),
        DeclareLaunchArgument('pi_4', default_value='0.9', description='Camera rotation param'),
        DeclareLaunchArgument('device_1_id', default_value='AUCF951007J', description='Camera 1 serial'),
        DeclareLaunchArgument('device_2_id', default_value='xx', description='Camera 2 serial'),
        namespaced_nodes,
    ])
