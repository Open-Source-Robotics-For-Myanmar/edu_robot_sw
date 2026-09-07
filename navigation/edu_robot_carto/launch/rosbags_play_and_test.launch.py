#!/usr/bin/env python3
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition

def generate_launch_description():

    use_composition = LaunchConfiguration('use_composition')
    use_slamtoolbox = LaunchConfiguration('use_slamtoolbox')
    use_sim_time = LaunchConfiguration('use_sim_time')

    declare_use_composition_cmd = DeclareLaunchArgument(
        'use_composition', default_value='False',
        description='Use composed bringup if True')
    
    declare_use_slamtoolbox_cmd = DeclareLaunchArgument(
        'use_slamtoolbox', default_value='False',
        description='Use use_slamtoolbox if True')
    
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time', default_value='False',
        description='Use use_sim_time if True')

    carto_slam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory("edu_robot_carto"), "launch", "cartographer_mapping.launch.py"
            )), condition=UnlessCondition(LaunchConfiguration('use_slamtoolbox')),
            launch_arguments={'use_sim_time': use_sim_time}.items() 
    )

    slamtoolbox_slam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(
            get_package_share_directory("edu_robot_nav2"), "launch", "slamtb_online_async_mapping.launch.py"
            )), condition=IfCondition(LaunchConfiguration('use_slamtoolbox')),
            launch_arguments={'use_sim_time': use_sim_time}.items() 
    )

    # composable true လုပ်ရန် 
    sim_navigation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory("edu_robot_nav2"),"launch","sim_mapping_navigation.launch.py"
            )]), launch_arguments={'use_composition': use_composition}.items()
    )
    
    # mode_assist = Node(
    #     package='',
    #     executable='',
    #     arguments=['mode','navi_mode'],
    # )

    return LaunchDescription(
        [
            # declare_use_composition_cmd,
            # declare_use_slamtoolbox_cmd,
            declare_use_sim_time_cmd,
            carto_slam,
            # slamtoolbox_slam,
            # sim_navigation
        ]
    )
    
# DONE
## carto နှင့် slamtoolbox switch ရန် use_slamtoolbox က default အားဖြင့် false 
## navigation ကို composable true လုပ်ရန် 
## yoyo_bringup/move_base_map.launch
### move_base_map.launch သည် move_base.launch နဲ့ မတူတာက global_costmap_param, local_costmap
### paremeter yaml ဖိုင်များမတူကြပါ။
### အဲ့တော့ ကျတော်တို့လည်း nav2_params_map.yaml နဲ့ nav2_params.yaml နှစ်မျိုးသုံးမယ်။

# TO DO
## လိုအပ်ရင် delay node သုံးရန်

### အဲ့တော့ ကျတော်တို့လည်း nav2_params_map.yaml နဲ့ nav2_params.yaml နှစ်မျိုးသုံးမယ်။
### အဲ့မှာ ဘာတွေပြောင်းရမလည်း algo လေ့လာရင်း ပြင်ဆင်ပါ။


# ORIGINAL

# / Nodes /
# carto

# / launch /
# move_base_map.launch