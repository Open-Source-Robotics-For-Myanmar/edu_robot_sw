import os
from launch import LaunchDescription
from launch.actions import GroupAction, IncludeLaunchDescription, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import FrontendLaunchDescriptionSource
from launch_ros.actions import PushRosNamespace
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', '')

    # ၁။ use_sim_time ကို argument အနေနဲ့ ကြေညာခြင်း
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation (Gazebo) clock if true'
    )

    # ၂။ rosbridge ဆီသို့ use_sim_time တန်ဖိုးကို လွှဲပေးခြင်း
    rosbridge_launch = IncludeLaunchDescription(
        FrontendLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('rosbridge_server'),
                'launch',
                'rosbridge_websocket_launch.xml'
            )
        ),
        launch_arguments={
            'use_sim_time': LaunchConfiguration('use_sim_time')
        }.items()
    )

    if rom_robot_namespace:
        namespaced = GroupAction(
            actions=[
                PushRosNamespace(rom_robot_namespace),
                rosbridge_launch,
            ]
        )
        return LaunchDescription([use_sim_time_arg, namespaced])
    else:
        return LaunchDescription([use_sim_time_arg, rosbridge_launch])