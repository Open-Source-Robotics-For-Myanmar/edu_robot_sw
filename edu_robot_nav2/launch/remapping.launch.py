# <launch>
#   <!-- Arguments -->
#   <arg name="multi_robot_name" default="1"/>
#   <arg name="model" default="$(env ROM_ROBOT_MODEL)" doc="model type [robot1, robot2, robot3, robot4]"/>
#   <arg name="map_file" default="/home/reeman/data/maps/default.yaml"/>
#   <arg name="move_forward_only" default="false"/>
#   <arg name="configuration_basename" default="yoyo_map_2d.lua"/>

#   <!-- CARTO -->
#   <node name="cartographer_node" pkg="cartographer_ros"
#       type="cartographer_node" args="
#           -configuration_directory $(find yoyo_bringup)/param
#           -configuration_basename $(arg configuration_basename)
#           -load_state_filename /home/reeman/data/maps/default.pbstream"
#           output="screen"
#       > 
#       <!-- output="screen"> -->
#     <!-- <remap from="/points2" to="/rslidar_points" /> -->
#   </node>

#   <!-- cartographer_occupancy_grid_node -->
#   <node pkg="cartographer_ros" type="cartographer_occupancy_grid_node"
#         name="cartographer_occupancy_grid_node" 
#         args="-resolution 0.05" />

#   <!-- move_base -->
#   <include file="$(find yoyo_bringup)/launch/includes/move_base_map.launch">
#     <arg name="model" value="$(arg model)" />
#   </include>

#   <node name="mode_assist" pkg="yoyo_bringup" type="mode_assist.py" output="screen">
#     <param name="mode" value="remap_mode" />
#   </node>

# </launch>