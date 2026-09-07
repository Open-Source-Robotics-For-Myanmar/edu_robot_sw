# rom_sdk_ws Namespace Compatibility Report

## ROM_ROBOT_NAMESPACE Pattern
- Environment variable: `ROM_ROBOT_NAMESPACE` (default: `default_robot1`)
- Launch files use `GroupAction` + `PushRosNamespace(namespace)` to wrap all nodes
- **Topics must be RELATIVE (no leading `/`)** to inherit namespace from `PushRosNamespace`

---

## Package Status

| Package | Status | Issues Fixed |
|---------|--------|-------------|
| rom_interfaces | ✅ N/A (interface only) | 0 |
| rom_obstacles_provider | ✅ Already compatible | 0 |
| which_maps | ✅ Already compatible | 0 |
| rom_waypoints_provider | ✅ Fixed | 18 |
| which_tasks | ✅ Fixed | 1 |

---

## Changes Made

### 1. which_tasks

**src/which_tasks_server.cpp** (line 36)
```diff
- std::string command_topic_name = "/diff_controller/cmd_vel_unstamped";
+ std::string command_topic_name = "diff_controller/cmd_vel_unstamped";
```

---

### 2. rom_waypoints_provider

#### C++ Source Fixes

**src/send_waypoints_goals.cpp** (lines 43, 46)
```diff
- client_ = rclcpp_action::create_client<FollowWaypoints>(this, "/follow_waypoints");
+ client_ = rclcpp_action::create_client<FollowWaypoints>(this, "follow_waypoints");

- marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/waypoint_markers", 10);
+ marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("waypoint_markers", 10);
```

**src/send_waypoints_custom_goals.cpp** (lines 36, 39)
```diff
- client_ = rclcpp_action::create_client<FollowWaypoints>(this, "/follow_waypoints");
+ client_ = rclcpp_action::create_client<FollowWaypoints>(this, "follow_waypoints");

- marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/waypoint_markers", 10);
+ marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("waypoint_markers", 10);
```

**src/send_waypoints_goals_loop.cpp** (lines 54, 57)
```diff
- client_ = rclcpp_action::create_client<FollowWaypoints>(this, "/follow_waypoints");
+ client_ = rclcpp_action::create_client<FollowWaypoints>(this, "follow_waypoints");

- marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("/waypoint_markers", 10);
+ marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("waypoint_markers", 10);
```

**src/robot_pose.cpp** (lines 13, 17)
```diff
- robot_pose_publisher_ = this->create_publisher<...>("/amcl_pose", 10);
+ robot_pose_publisher_ = this->create_publisher<...>("amcl_pose", 10);

- odom_subscriber_ = this->create_subscription<...>("/odom", 10, ...);
+ odom_subscriber_ = this->create_subscription<...>("odom", 10, ...);
```

**src/construct_xml_server_bt.cpp** (12 occurrences across 3 blocks)
```diff
  BT XML generation — server_name attributes:
- server_name="/follow_path"  → server_name="follow_path"
- server_name="/spin"         → server_name="spin"
- server_name="/wait"         → server_name="wait"
- server_name="/backup"       → server_name="backup"
```
Fixed in 3 identical blocks: lines ~148-176, ~371-384, ~573-586

#### Launch File Updates (ROM_ROBOT_NAMESPACE + GroupAction + PushRosNamespace)

**launch/send_waypoints_all_goals.launch.py**
```diff
+ import os
+ rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
+ GroupAction([ PushRosNamespace(rom_robot_namespace), Node(...) ])
```

**launch/send_waypoints_custom_goals.launch.py**
```diff
+ import os
+ rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
+ GroupAction([ PushRosNamespace(rom_robot_namespace), Node(...) ])
```

**launch/send_waypoints_all_goals_loop.launch.py**
```diff
+ import os
+ rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
+ GroupAction([ PushRosNamespace(rom_robot_namespace), Node(...) ])
```

**launch/send_waypoints_custom_goals_loop.launch.py**
```diff
+ import os
+ rom_robot_namespace = os.environ.get('ROM_ROBOT_NAMESPACE', 'default_robot1')
+ GroupAction([ PushRosNamespace(rom_robot_namespace), Node(...) ])
```

---

## Already Correct (No Changes Needed)

- **rom_interfaces** — pure message/service definitions, no topics
- **rom_obstacles_provider** — `line_obstacles`, `add_line_obstacles` already relative ✅
- **which_maps** — `which_name`, `which_nav`, `cmd_vel_qt_to_twist`, `which_vel`, `map_bfp_publisher` already relative ✅
- **rom_waypoints_provider/src/path_runner.cpp** — `navigate_through_poses`, `path_runner/cancel` already relative ✅
- **rom_waypoints_provider/src/send_waypoints_server.cpp** — already relative ✅
- **rom_waypoints_provider/src/construct_yaml_server.cpp** — `construct_yaml` already relative ✅

---

## Total: 19 fixes across 2 packages (5 C++ files + 4 launch files)
