# generated from rosidl_cmake/cmake/rosidl_cmake_aggregate_target-extras.cmake.in

# Create a convenience aggregate target rom_interfaces::rom_interfaces
# that links all generated interface targets, so downstream packages can use
# a single modern CMake target name instead of ${rom_interfaces_TARGETS}.
if(rom_interfaces_TARGETS AND NOT TARGET rom_interfaces::rom_interfaces)
  add_library(rom_interfaces::rom_interfaces INTERFACE IMPORTED)
  set_target_properties(rom_interfaces::rom_interfaces PROPERTIES
    INTERFACE_LINK_LIBRARIES "${rom_interfaces_TARGETS}")
endif()
