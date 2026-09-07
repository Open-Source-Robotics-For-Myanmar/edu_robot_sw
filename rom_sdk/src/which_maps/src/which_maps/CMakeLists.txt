cmake_minimum_required(VERSION 3.8)
project(which_maps)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rom_interfaces REQUIRED)
find_package(std_msgs REQUIRED)
find_package(example_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)

find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(cartographer_ros_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()


add_executable(which_maps_server src/which_maps_server.cpp)
ament_target_dependencies(which_maps_server rclcpp rom_interfaces)

add_executable(which_name_server src/which_name_server.cpp)
ament_target_dependencies(which_name_server rclcpp rom_interfaces)

add_executable(which_vel_server src/which_vel_server.cpp)
ament_target_dependencies(which_vel_server rclcpp rom_interfaces)

add_executable(which_nav_server src/which_navs_server.cpp)
ament_target_dependencies(which_nav_server rclcpp rom_interfaces)

# add_executable(ntmgr src/rom_node_thread_mgr.cpp)
# ament_target_dependencies(ntmgr rclcpp rom_interfaces)

add_executable(which_maps_client src/which_maps_client.cpp)
ament_target_dependencies(which_maps_client rclcpp rom_interfaces)

add_executable(map_base_footprint_pub src/map_base_footprint_pub.cpp)
ament_target_dependencies(map_base_footprint_pub tf2 tf2_ros geometry_msgs rclcpp rom_interfaces)

add_executable(relocate_server src/relocate_server.cpp)
ament_target_dependencies(relocate_server rclcpp geometry_msgs cartographer_ros_msgs ament_index_cpp)

install(
  DIRECTORY maps       # Source directory
  DESTINATION share/${PROJECT_NAME}  # Destination directory in install space
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(ROM_DEBUG)
endif()

# install(TARGETS
#   server
#   client
#   DESTINATION lib/${PROJECT_NAME})

#add_executable(add_two_ints_server src/add_two_ints_server.cpp)
#ament_target_dependencies(add_two_ints_server rclcpp example_interfaces)

install(TARGETS
  #add_two_ints_server
  map_base_footprint_pub
  which_maps_server
  which_name_server
  which_maps_client
  which_nav_server
  which_vel_server
  relocate_server
  DESTINATION lib/${PROJECT_NAME})

ament_package()
