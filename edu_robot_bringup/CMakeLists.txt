cmake_minimum_required(VERSION 3.8)
project(edu_robot_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclpy REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

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


add_executable(mapping src/mapping.cpp)
ament_target_dependencies(mapping rclcpp std_msgs)

add_executable(nav2 src/nav.cpp)
ament_target_dependencies(nav2 rclcpp std_msgs)

add_executable(remapping src/remapping.cpp)
ament_target_dependencies(remapping rclcpp std_msgs)

add_executable(trigger src/trigger.cpp)
ament_target_dependencies(trigger rclcpp std_msgs)


install(TARGETS
  mapping
  nav2
  remapping
  trigger
  DESTINATION lib/${PROJECT_NAME})


install(
    DIRECTORY
      launch
    DESTINATION
      share/${PROJECT_NAME}/
  )


# Install Python executables
install(PROGRAMS

  scripts/tk.py
  scripts/switch_mode.py
  # launch/mapping.launch.py
  DESTINATION lib/${PROJECT_NAME}
)


ament_package()
