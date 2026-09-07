import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import ThisLaunchFileDir
from launch.conditions import IfCondition

# ROM_ROBOT_NAMESPACE environment variable မှ namespace ရယူ
robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', '')


def generate_launch_description():
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    # use_rviz     = LaunchConfiguration('use_rviz', default='false')
    
    edu_robot_carto_pkg = get_package_share_directory('edu_robot_carto')
    cartographer_config_dir = LaunchConfiguration('cartographer_config_dir', default=os.path.join(
                                                  edu_robot_carto_pkg, 'config'))
    configuration_basename = LaunchConfiguration('configuration_basename',
                                                 default='edu_robot_map_2d.lua')
    load_state_filename = LaunchConfiguration('load_state_filename', default='/home/buc_robot/data/maps/a2.pbstream')
    
    resolution = LaunchConfiguration('resolution', default='0.05')
    publish_period_sec = LaunchConfiguration('publish_period_sec', default='1.0')

    # rviz_config_dir = os.path.join(get_package_share_directory('edu_robot_carto'),
    #                                'rviz', 'edu_robot_cartographer.rviz')

    return LaunchDescription([
        DeclareLaunchArgument(
            'cartographer_config_dir',
            default_value=cartographer_config_dir,
            description='Full path to config file to load'),
        DeclareLaunchArgument(
            'configuration_basename',
            default_value=configuration_basename,
            description='Name of lua file for cartographer'),
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation (Gazebo) clock if true'),

        Node(
            package='cartographer_ros',
            executable='cartographer_node',
            name='cartographer_node',
            namespace=robot_namespace,
            output='screen',
            parameters=[{'use_sim_time': use_sim_time}],
            # remappings=[('/odom', '/diff_controller/odom')],
            arguments=['-configuration_directory', '/home/buc_robot/software_ws/install/edu_robot_carto/share/edu_robot_carto/config/',
                       '-configuration_basename', configuration_basename,
                       '-load_state_filename', load_state_filename],
            ),

        DeclareLaunchArgument(
            'resolution',
            default_value=resolution,
            description='Resolution of a grid cell in the published occupancy grid'),

        DeclareLaunchArgument(
            'publish_period_sec',
            default_value=publish_period_sec,
            description='OccupancyGrid publishing period'),

        IncludeLaunchDescription( 
            PythonLaunchDescriptionSource([ThisLaunchFileDir(), '/occupancy_grid.launch.py']),
            launch_arguments={'use_sim_time': use_sim_time, 'resolution': resolution,
                              'publish_period_sec': publish_period_sec,
                              'robot_namespace': robot_namespace}.items(),
        ),

        # Node(
        #     package='rviz2',
        #     executable='rviz2',
        #     name='rviz2',
        #     arguments=['-d', rviz_config_dir],
        #     parameters=[{'use_sim_time': use_sim_time}],
        #     output='screen',
        #     condition=IfCondition(use_rviz)
        #     ),
               
    ])
