cmake_minimum_required(VERSION 3.8)
project(ar_robot_launch)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()
