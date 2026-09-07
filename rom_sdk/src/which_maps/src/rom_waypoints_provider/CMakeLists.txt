cmake_minimum_required(VERSION 3.8)
project(rom_waypoints_provider)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(rom_interfaces REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

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

# add_executable(send_waypoints_goals src/send_waypoints_goals.cpp)
# target_link_libraries(send_waypoints_goals yaml-cpp)
# ament_target_dependencies(send_waypoints_goals rclcpp rclcpp_action nav2_msgs geometry_msgs visualization_msgs yaml-cpp ament_index_cpp)

# add_executable(send_waypoints_goals_loop src/send_waypoints_goals_loop.cpp)
# target_link_libraries(send_waypoints_goals_loop yaml-cpp)
# ament_target_dependencies(send_waypoints_goals_loop rclcpp rclcpp_action nav2_msgs geometry_msgs visualization_msgs yaml-cpp ament_index_cpp)

# add_executable(construct_yaml_server src/construct_yaml_server.cpp)
# target_link_libraries(construct_yaml_server yaml-cpp)
# ament_target_dependencies(construct_yaml_server rclcpp rclcpp_action nav2_msgs geometry_msgs visualization_msgs yaml-cpp ament_index_cpp rom_interfaces)



add_executable(construct_xml_server_bt src/construct_xml_server_bt.cpp)
target_link_libraries(construct_xml_server_bt yaml-cpp)
ament_target_dependencies(construct_xml_server_bt rclcpp std_srvs rclcpp_action nav2_msgs geometry_msgs visualization_msgs yaml-cpp ament_index_cpp rom_interfaces)



# add_executable(send_waypoints_server src/send_waypoints_server.cpp)
# target_link_libraries(send_waypoints_server yaml-cpp)
# ament_target_dependencies(send_waypoints_server rclcpp rclcpp_action nav2_msgs geometry_msgs visualization_msgs yaml-cpp ament_index_cpp rom_interfaces)

add_executable(robot_pose_publisher src/robot_pose.cpp)
ament_target_dependencies(robot_pose_publisher rclcpp geometry_msgs tf2 tf2_ros nav_msgs tf2_geometry_msgs)

add_executable(path_runner src/path_runner.cpp)
target_link_libraries(path_runner yaml-cpp)
ament_target_dependencies(path_runner rclcpp rclcpp_action geometry_msgs nav2_msgs tf2_geometry_msgs yaml-cpp std_srvs)


install(TARGETS 
  # send_waypoints_goals
  # send_waypoints_goals_loop 
  robot_pose_publisher
  # construct_yaml_server
  # send_waypoints_server
  construct_xml_server_bt
  path_runner
DESTINATION lib/${PROJECT_NAME})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(ROM_DEBUG)
endif()

install(
  DIRECTORY 
    config
    launch
  DESTINATION share/${PROJECT_NAME}  # Destination directory in install space
)

ament_package()
