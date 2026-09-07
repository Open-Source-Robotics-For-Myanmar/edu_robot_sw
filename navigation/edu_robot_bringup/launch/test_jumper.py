import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, IncludeLaunchDescription, Shutdown
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():

    pkg_name = "edu_robot_bringup"

    return LaunchDescription([
        DeclareLaunchArgument('mode', default_value='mode1'),
        
        IncludeLaunchDescription(
            os.path.join(pkg_name,"launch","mapping.launch.py")
            condition=IfCondition(LaunchConfiguration('mode').equals('mode1'))
        ),

        IncludeLaunchDescription(
            'path_to_mode2_launch_file.launch.py',
            condition=IfCondition(LaunchConfiguration('mode').equals('mode2'))
        ),

        IncludeLaunchDescription(
            'path_to_mode3_launch_file.launch.py',
            condition=IfCondition(LaunchConfiguration('mode').equals('mode3'))
        ),

        LogInfo(
            msg='Launch mode selected: {}'.format(LaunchConfiguration('mode'))
        ),
    ])

def main(args=None):
    ls = LaunchService()
    ld = generate_launch_description()
    ls.include_launch_description(ld)
    return ls.run()

if __name__ == '__main__':
    main()