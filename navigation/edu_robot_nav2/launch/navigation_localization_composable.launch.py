import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, GroupAction,
                            IncludeLaunchDescription, SetEnvironmentVariable)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node
from launch_ros.actions import PushRosNamespace
from launch_ros.descriptions import ParameterFile
from nav2_common.launch import RewrittenYaml, ReplaceString

#TURTLEBOT3_MODEL = os.environ['TURTLEBOT3_MODEL']


def generate_launch_description():
    rom_robot_name = os.environ.get('ROM_ROBOT_MODEL', 'edu_robot')
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    use_rviz = LaunchConfiguration('use_rviz', default='false')
    use_3d_cam = LaunchConfiguration('use_3d_cam', default='false')

    map_dir = LaunchConfiguration('map', default='/home/buc_robot/data/maps/a2.yaml')

    param_file_name = 'nav2_params.yaml'
    param_dir = LaunchConfiguration(
        'params_file',
        default=os.path.join(get_package_share_directory(f'{rom_robot_name}_nav2'), 'config', param_file_name))

    nav2_launch_file_dir = os.path.join(get_package_share_directory(f'{rom_robot_name}_nav2'), 'launch/include')

    dabai_launch_file_dir = os.path.join(get_package_share_directory('rom_dabai_3d_camera'), 'launch')

    rviz_config_dir = os.path.join(
        get_package_share_directory(f'{rom_robot_name}_nav2'),
        'rviz',
        'nav2_default_view.rviz')

    return LaunchDescription([
        DeclareLaunchArgument(
            'map',
            default_value=map_dir,
            description='Full path to map file to load'),

        DeclareLaunchArgument(
            'params_file',
            default_value=param_dir,
            description='Full path to param file to load'),

        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation (Gazebo) clock if true'),

        DeclareLaunchArgument(
            'use_3d_cam',
            default_value='false',
            description='Launch Dabai 3D camera nodes if true'),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(dabai_launch_file_dir, 'pointcloud_mini_dabai_dabai.launch.py')
            ),
            condition=IfCondition(use_3d_cam),
        ),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([nav2_launch_file_dir, '/bringup_launch.py']),
            launch_arguments={
                'map': map_dir,
                'use_sim_time': use_sim_time,
                'params_file': param_dir,
                'namespace': rom_robot_namespace,
                'use_namespace': 'true' if rom_robot_namespace else 'false'}.items(),
        ),

        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', rviz_config_dir],
            parameters=[{'use_sim_time': use_sim_time}],
            condition=IfCondition(use_rviz),
            output='screen'),
    ])
