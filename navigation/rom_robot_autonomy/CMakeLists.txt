cmake_minimum_required(VERSION 3.8)
project(rom_robot_autonomy)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

install(DIRECTORY
trees launch config
DESTINATION share/${PROJECT_NAME}
)

set(BEHAVIOR_SOURCES
src/navigation_behaviors.cpp
)

set(TARGET_DEPENDS
  rclcpp
  rclcpp_action
  nav2_msgs
  nav2_behavior_tree
  behaviortree_cpp_v3
  ament_index_cpp
  yaml-cpp
  tf2
  tf2_geometry_msgs
  std_srvs
)

include_directories(include)


# add_executable(autonomy_waypoints_mode_w_cancel src/main_waypoints_mode_with_cancel_net_host.cpp ${BEHAVIOR_SOURCES})
add_executable(autonomy_waypoints_mode_w_cancel src/main_waypoints_mode_with_cancel.cpp ${BEHAVIOR_SOURCES})
ament_target_dependencies(autonomy_waypoints_mode_w_cancel ${TARGET_DEPENDS})
target_link_libraries(autonomy_waypoints_mode_w_cancel ${YAML_CPP_LIBRARIES})

add_executable(autonomy_patrol_mode_w_cancel src/main_patrol_mode_with_cancel.cpp ${BEHAVIOR_SOURCES})
ament_target_dependencies(autonomy_patrol_mode_w_cancel ${TARGET_DEPENDS})
target_link_libraries(autonomy_patrol_mode_w_cancel ${YAML_CPP_LIBRARIES})

add_executable(autonomy_service_mode_w_cancel src/main_service_mode_with_cancel.cpp ${BEHAVIOR_SOURCES})
ament_target_dependencies(autonomy_service_mode_w_cancel ${TARGET_DEPENDS})
target_link_libraries(autonomy_service_mode_w_cancel ${YAML_CPP_LIBRARIES})

install(TARGETS
  autonomy_waypoints_mode_w_cancel
  autonomy_patrol_mode_w_cancel
  autonomy_service_mode_w_cancel
  DESTINATION lib/${PROJECT_NAME}
)

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

ament_package()
