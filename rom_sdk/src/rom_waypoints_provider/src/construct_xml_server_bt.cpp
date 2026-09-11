#define IS_PRODUCTION 1

#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rom_interfaces/msg/construct_yaml.hpp"
#include "rom_interfaces/srv/construct_yaml.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_srvs/srv/set_bool.hpp"

#include <algorithm>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef ROM_DYNAMICS_UNUSED
#define ROM_DYNAMICS_UNUSED(x) (void)(x)
#endif

// ============================================================================
// File Path Definitions
// ============================================================================
namespace paths {
const std::string TREE_MODELS_SOURCE =
    "/home/buc_robot/data/trees/tree_nodes_models.xml";

const std::string WAYPOINTS_XML =
    "/home/buc_robot/data/trees/waypoints_mode.xml";
const std::string WAYPOINTS_YAML =
    "/home/buc_robot/data/waypoints/waypoints_mode.yaml";

const std::string SERVICE_XML = "/home/buc_robot/data/trees/service_mode.xml";
const std::string SERVICE_YAML =
    "/home/buc_robot/data/waypoints/service_mode.yaml";

const std::string PATROL_XML = "/home/buc_robot/data/trees/patrol_mode.xml";
const std::string PATROL_YAML =
    "/home/buc_robot/data/waypoints/patrol_mode.yaml";

const std::string PATH_YAML = "/home/buc_robot/data/waypoints/path_mode.yaml";
} // namespace paths

// Global variables
const std::string rom_robot_namespace = std::getenv("ROM_ROBOT_NAMESPACE")
                                            ? std::getenv("ROM_ROBOT_NAMESPACE")
                                            : "";
rclcpp::Publisher<rom_interfaces::msg::ConstructYaml>::SharedPtr publisher_;
rclcpp::Client<std_srvs::srv::SetBool>::SharedPtr bt_client;
std::shared_ptr<rclcpp::Node> bt_stop_node;
bool debug_mode_ = false;

// ============================================================================
// Helper Functions: Behavior Tree Control
// ============================================================================

// ယခင် run နေတဲ့ bt_tree ရှိခဲ့ရင် အရင်ဆုံးရပ်တန့်ဖို့အတွက် service request လုပ်မယ်။
bool stop_behavior_tree() {
  RCLCPP_INFO(rclcpp::get_logger("bt stop node "),
              "Waiting for bt_stop service to be available...");

  if (!bt_client->wait_for_service(std::chrono::seconds(3))) {
    RCLCPP_WARN(rclcpp::get_logger("bt stop node "),
                "Service /edu_robot/bt_stop is not available after waiting for "
                "3 seconds.");
    return false;
  }

  RCLCPP_INFO(rclcpp::get_logger("bt stop node "),
              "Service /edu_robot/bt_stop is available!");

  auto bt_stop_request = std::make_shared<std_srvs::srv::SetBool::Request>();
  bt_stop_request->data = true;

  auto future = bt_client->async_send_request(bt_stop_request);

  if (rclcpp::spin_until_future_complete(bt_stop_node, future) ==
      rclcpp::FutureReturnCode::SUCCESS) {
    RCLCPP_INFO(rclcpp::get_logger("bt stop node "),
                "Received response: success");
    return true;
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("bt stop node "),
                 "Failed to receive response.");
    return false;
  }
}

// tree_nodes_models.xml source file ကို xml ဖိုင်ထဲသို့ ထည့်သွင်းခြင်း
bool append_tree_models(std::ofstream &xml_file) {
  std::ifstream src(paths::TREE_MODELS_SOURCE, std::ios::in);
  if (!src.is_open()) {
    if (debug_mode_) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                         "tree_nodes_models.xml source file error!!");
    }
    return false;
  }
  xml_file << src.rdbuf();
  src.close();
  return true;
}

// ============================================================================
// Helper Functions: YAML I/O
// ============================================================================

// Waypoints များကို YAML file အဖြစ် သိမ်းဆည်းခြင်း
bool write_waypoints_yaml(
    const std::string &yaml_path, const std::vector<std::string> &pose_names,
    const std::vector<geometry_msgs::msg::PoseStamped> &poses,
    const std::vector<geometry_msgs::msg::Pose> &scene_poses,
    const std::string &mode_name) {
  if (!std::filesystem::exists(yaml_path)) {
    if (debug_mode_) {
      RCLCPP_INFO_STREAM(
          rclcpp::get_logger("xml constructor ( yaml construct )"),
          mode_name << ".yaml file doesn't exists: ");
    }
    return false;
  }

  std::ofstream yaml_file(yaml_path, std::ios::trunc);
  if (!yaml_file.is_open()) {
    if (debug_mode_) {
      RCLCPP_ERROR(rclcpp::get_logger("xml constructor ( yaml construct )"),
                   "%s.yaml file open error!!", mode_name.c_str());
    }
    return false;
  }

  yaml_file << "waypoints:\n";
  for (int i = static_cast<int>(pose_names.size()) - 1; i >= 0; --i) {
    yaml_file << "  - name: " << pose_names[i] << "\n";
    yaml_file << "    frame_id: " << "map\n";
    yaml_file << "    pose:\n";
    yaml_file << "      position:\n";
    yaml_file << "        x: " << poses[i].pose.position.x << "\n";
    yaml_file << "        y: " << poses[i].pose.position.y << "\n";
    yaml_file << "        z: " << poses[i].pose.position.z << "\n";
    yaml_file << "      orientation:\n";
    yaml_file << "        x: " << poses[i].pose.orientation.x << "\n";
    yaml_file << "        y: " << poses[i].pose.orientation.y << "\n";
    yaml_file << "        z: " << poses[i].pose.orientation.z << "\n";
    yaml_file << "        w: " << poses[i].pose.orientation.w << "\n";
    yaml_file << "    scene_poses:\n";
    yaml_file << "      x: " << scene_poses[i].position.x << "\n";
    yaml_file << "      y: " << scene_poses[i].position.y << "\n";
    yaml_file << "      phi: " << scene_poses[i].orientation.w << "\n";

    if (debug_mode_) {
      RCLCPP_INFO(rclcpp::get_logger("xml constructor ( yaml construct )"),
                  "%s", pose_names[i].c_str());
    }
  }

  yaml_file.flush();
  yaml_file.close();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor ( yaml construct )"),
                       mode_name << ".yaml created successfully!");
  }
  return true;
}

// YAML file မှ waypoints list ကို parse ပြုလုပ်ခြင်း
bool parse_waypoints_yaml(
    const std::string &yaml_path, const std::string &tag_name,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  response->status = -1;

  if (!std::filesystem::exists(yaml_path)) {
    RCLCPP_WARN(rclcpp::get_logger("xml constructor"),
                "%s: yaml file does not exist: %s", tag_name.c_str(),
                yaml_path.c_str());
    return false;
  }

  std::ifstream in(yaml_path);
  if (!in.is_open()) {
    RCLCPP_ERROR(rclcpp::get_logger("xml constructor"),
                 "%s: failed to open yaml file: %s", tag_name.c_str(),
                 yaml_path.c_str());
    return false;
  }

  auto trim = [](std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
              return !std::isspace(ch);
            }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
  };

  std::string line;
  std::string current_name;
  geometry_msgs::msg::PoseStamped cur_pose_stamped;
  geometry_msgs::msg::Pose cur_scene_pose;
  bool in_pose = false;
  bool in_position = false;
  bool in_orientation = false;
  bool in_scene = false;

  while (std::getline(in, line)) {
    std::string s = line;
    trim(s);

    if (s.rfind("- name:", 0) == 0) {
      if (!current_name.empty()) {
        response->pose_names.push_back(current_name);
        response->poses.push_back(cur_pose_stamped);
        response->scene_poses.push_back(cur_scene_pose);
        cur_pose_stamped = geometry_msgs::msg::PoseStamped();
        cur_scene_pose = geometry_msgs::msg::Pose();
      }
      auto pos = s.find(":");
      if (pos != std::string::npos) {
        current_name = s.substr(pos + 1);
        trim(current_name);
      } else {
        current_name.clear();
      }
      cur_pose_stamped.header.frame_id = "map";
      in_pose = in_position = in_orientation = in_scene = false;
    } else if (s.rfind("pose:", 0) == 0) {
      in_pose = true;
      in_position = in_orientation = false;
      in_scene = false;
    } else if (s.rfind("position:", 0) == 0 && in_pose) {
      in_position = true;
      in_orientation = false;
      in_scene = false;
    } else if (s.rfind("orientation:", 0) == 0 && in_pose) {
      in_orientation = true;
      in_position = false;
      in_scene = false;
    } else if (s.rfind("scene_poses:", 0) == 0) {
      in_scene = true;
      in_pose = in_position = in_orientation = false;
    } else if (in_position && (s.rfind("x:", 0) == 0 || s.rfind("y:", 0) == 0 ||
                               s.rfind("z:", 0) == 0)) {
      auto pos = s.find(":");
      std::string key = s.substr(0, pos);
      std::string val = (pos == std::string::npos) ? "" : s.substr(pos + 1);
      trim(key);
      trim(val);
      try {
        double v = std::stod(val);
        if (key == "x")
          cur_pose_stamped.pose.position.x = v;
        else if (key == "y")
          cur_pose_stamped.pose.position.y = v;
        else if (key == "z")
          cur_pose_stamped.pose.position.z = v;
      } catch (...) {
      }
    } else if (in_orientation &&
               (s.rfind("x:", 0) == 0 || s.rfind("y:", 0) == 0 ||
                s.rfind("z:", 0) == 0 || s.rfind("w:", 0) == 0)) {
      auto pos = s.find(":");
      std::string key = s.substr(0, pos);
      std::string val = (pos == std::string::npos) ? "" : s.substr(pos + 1);
      trim(key);
      trim(val);
      try {
        double v = std::stod(val);
        if (key == "x")
          cur_pose_stamped.pose.orientation.x = v;
        else if (key == "y")
          cur_pose_stamped.pose.orientation.y = v;
        else if (key == "z")
          cur_pose_stamped.pose.orientation.z = v;
        else if (key == "w")
          cur_pose_stamped.pose.orientation.w = v;
      } catch (...) {
      }
    } else if (in_scene && (s.rfind("x:", 0) == 0 || s.rfind("y:", 0) == 0 ||
                            s.rfind("phi:", 0) == 0)) {
      auto pos = s.find(":");
      std::string key = s.substr(0, pos);
      std::string val = (pos == std::string::npos) ? "" : s.substr(pos + 1);
      trim(key);
      trim(val);
      try {
        double v = std::stod(val);
        if (key == "x")
          cur_scene_pose.position.x = v;
        else if (key == "y")
          cur_scene_pose.position.y = v;
        else if (key == "phi")
          cur_scene_pose.orientation.w = v;
      } catch (...) {
      }
    }
  }

  if (!current_name.empty()) {
    response->pose_names.push_back(current_name);
    response->poses.push_back(cur_pose_stamped);
    response->scene_poses.push_back(cur_scene_pose);
  }

  in.close();
  response->status = 1;

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       tag_name << ": parsed " << response->pose_names.size()
                                << " waypoints");
  }
  return true;
}

// ============================================================================
// Helper Functions: XML Snippets (7-Stage Intelligent Recovery)
// ============================================================================

// Waypoints mode အတွက် direct tag format (7-Stage Recovery)
void write_recovery_subtree_waypoints(std::ofstream &xml_file,
                                      const std::string &pose_name) {
  xml_file << "      <RecoveryNode name=\"NavigateRecovery\"\n";
  xml_file << "                    number_of_retries=\"-1\">\n";
  xml_file << "        <PipelineSequence name=\"NavigateWithReplanning\">\n";
  xml_file << "          <RateController hz=\"1.0\">\n";
  xml_file << "            <RecoveryNode name=\"ComputePathToPose\"\n";
  xml_file << "                          number_of_retries=\"1\">\n";
  xml_file << "              <ComputePathToPose goal=\"{" << pose_name
           << "}\"\n";
  xml_file << "                                 start=\"\"\n";
  xml_file << "                                 planner_id=\"GridBased\"\n";
  xml_file << "                                 "
              "server_name=\"compute_path_to_pose\"\n";
  xml_file << "                                 server_timeout=\"10.0\"\n";
  xml_file << "                                 path=\"{path}\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearGlobalCostmap-Context\"\n";
  xml_file << "                                  "
              "service_name=\"global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </RecoveryNode>\n";
  xml_file << "          </RateController>\n";
  xml_file << "          <RecoveryNode name=\"FollowPath\"\n";
  xml_file << "                        number_of_retries=\"1\">\n";
  xml_file << "            <FollowPath controller_id=\"FollowPath\"\n";
  xml_file << "                        path=\"{path}\"\n";
  xml_file << "                        goal_checker_id=\"\"\n";
  xml_file << "                        server_name=\"follow_path\"\n";
  xml_file << "                        server_timeout=\"10.0\"/>\n";
  xml_file
      << "            <ClearEntireCostmap name=\"ClearLocalCostmap-Context\"\n";
  xml_file << "                                "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                server_timeout=\"10.0\"/>\n";
  xml_file << "          </RecoveryNode>\n";
  xml_file << "        </PipelineSequence>\n";
  // 7-Stage Intelligent Recovery — No BackUp (front LiDAR only)
  xml_file << "        <ReactiveFallback name=\"RecoveryFallback\">\n";
  xml_file << "          <GoalUpdated/>\n";
  xml_file << "          <RoundRobin name=\"RecoveryActions\">\n";
  // Stage 1: Clear local costmap only
  xml_file
      << "            <ClearEntireCostmap name=\"ClearLocalCostmap-Subtree\"\n";
  xml_file << "                                "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                server_timeout=\"10.0\"/>\n";
  // Stage 2: Small spin 45deg + clear local
  xml_file << "            <Sequence name=\"SpinSmallAndClear\">\n";
  xml_file << "              <Spin spin_dist=\"0.785\"\n";
  xml_file << "                    time_allowance=\"6.0\"\n";
  xml_file << "                    server_name=\"spin\"\n";
  xml_file << "                    server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearLocalCostmap-AfterSpin45\"\n";
  xml_file << "                                  "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </Sequence>\n";
  // Stage 3: Wait 3s
  xml_file << "            <Wait wait_duration=\"3\"\n";
  xml_file << "                  server_name=\"wait\"\n";
  xml_file << "                  server_timeout=\"10.0\"/>\n";
  // Stage 4: Spin right 90deg + clear all
  xml_file << "            <Sequence name=\"SpinRightAndClearAll\">\n";
  xml_file << "              <Spin spin_dist=\"1.57\"\n";
  xml_file << "                    time_allowance=\"10.0\"\n";
  xml_file << "                    server_name=\"spin\"\n";
  xml_file << "                    server_timeout=\"15.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearLocalCostmap-AfterSpin90R\"\n";
  xml_file << "                                  "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearGlobalCostmap-AfterSpin90R\"\n";
  xml_file << "                                  "
              "service_name=\"global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </Sequence>\n";
  // Stage 5: Spin left 90deg + clear all
  xml_file << "            <Sequence name=\"SpinLeftAndClearAll\">\n";
  xml_file << "              <Spin spin_dist=\"-1.57\"\n";
  xml_file << "                    time_allowance=\"10.0\"\n";
  xml_file << "                    server_name=\"spin\"\n";
  xml_file << "                    server_timeout=\"15.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearLocalCostmap-AfterSpin90L\"\n";
  xml_file << "                                  "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearGlobalCostmap-AfterSpin90L\"\n";
  xml_file << "                                  "
              "service_name=\"global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </Sequence>\n";
  // Stage 6: Wait 5s + full clear
  xml_file << "            <Sequence name=\"WaitAndFullClear\">\n";
  xml_file << "              <Wait wait_duration=\"5\"\n";
  xml_file << "                    server_name=\"wait\"\n";
  xml_file << "                    server_timeout=\"15.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearLocalCostmap-AfterWait\"\n";
  xml_file << "                                  "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearGlobalCostmap-AfterWait\"\n";
  xml_file << "                                  "
              "service_name=\"global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </Sequence>\n";
  // Stage 7: Spin 180deg + clear all
  xml_file << "            <Sequence name=\"LastResortFullReset\">\n";
  xml_file << "              <Spin spin_dist=\"3.14\"\n";
  xml_file << "                    time_allowance=\"15.0\"\n";
  xml_file << "                    server_name=\"spin\"\n";
  xml_file << "                    server_timeout=\"20.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearLocalCostmap-LastResort\"\n";
  xml_file << "                                  "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap "
              "name=\"ClearGlobalCostmap-LastResort\"\n";
  xml_file << "                                  "
              "service_name=\"global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << "            </Sequence>\n";
  xml_file << "          </RoundRobin>\n";
  xml_file << "        </ReactiveFallback>\n";
  xml_file << "      </RecoveryNode>\n";
}

// Service / Patrol mode အတွက် Control/Action tag format (7-Stage Recovery)
void write_recovery_subtree_action_format(std::ofstream &xml_file,
                                          const std::string &pose_name) {
  xml_file << "                <Control ID=\"RecoveryNode\" "
              "name=\"NavigateRecovery\" number_of_retries=\"-1\">\n";
  xml_file << "                    <Control ID=\"PipelineSequence\" "
              "name=\"NavigateWithReplanning\">\n";
  xml_file << "                        <Decorator ID=\"RateController\" "
              "hz=\"1.0\">\n";
  xml_file << "                            <Control ID=\"RecoveryNode\" "
              "name=\"ComputePathToPose\" number_of_retries=\"1\">\n";
  xml_file
      << "                                <Action ID=\"ComputePathToPose\" "
         "goal=\"{"
      << pose_name
      << "}\" start=\"\" path=\"{path}\" planner_id=\"GridBased\" "
         "server_name=\"compute_path_to_pose\" server_timeout=\"10.0\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearGlobalCostmap-Context\" "
         "server_timeout=\"10.0\" "
         "service_name=\"global_costmap/clear_entirely_global_costmap\"/>\n";
  xml_file << "                            </Control>\n";
  xml_file << "                        </Decorator>\n";
  xml_file << "                        <Control ID=\"RecoveryNode\" "
              "name=\"FollowPath\" number_of_retries=\"1\">\n";
  xml_file << "                            <Action ID=\"FollowPath\" "
              "controller_id=\"FollowPath\" goal_checker_id=\"\" "
              "path=\"{path}\" server_name=\"follow_path\" "
              "server_timeout=\"10.0\"/>\n";
  xml_file << "                            <Action ID=\"ClearEntireCostmap\" "
              "name=\"ClearLocalCostmap-Context\" "
              "server_timeout=\"10.0\" "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file << "                        </Control>\n";
  xml_file << "                    </Control>\n";
  // 7-Stage Intelligent Recovery — No BackUp (front LiDAR only)
  xml_file
      << "                    <ReactiveFallback name=\"RecoveryFallback\">\n";
  xml_file << "                        <Condition ID=\"GoalUpdated\"/>\n";
  xml_file << "                        <Control ID=\"RoundRobin\" "
              "name=\"RecoveryActions\">\n";
  // Stage 1: Clear local costmap only
  xml_file << "                            <Action ID=\"ClearEntireCostmap\" "
              "name=\"ClearLocalCostmap-Subtree\" "
              "server_timeout=\"10.0\" "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  // Stage 2: Small spin 45deg + clear local
  xml_file
      << "                            <Sequence name=\"SpinSmallAndClear\">\n";
  xml_file << "                                <Action ID=\"Spin\" "
              "server_name=\"spin\" server_timeout=\"10.0\" "
              "spin_dist=\"0.785\" time_allowance=\"6.0\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearLocalCostmap-AfterSpin45\" "
         "server_timeout=\"10.0\" "
         "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file << "                            </Sequence>\n";
  // Stage 3: Wait 3s
  xml_file
      << "                            <Action ID=\"Wait\" server_name=\"wait\" "
         "server_timeout=\"10.0\" wait_duration=\"3\"/>\n";
  // Stage 4: Spin right 90deg + clear local + global
  xml_file << "                            <Sequence "
              "name=\"SpinRightAndClearAll\">\n";
  xml_file << "                                <Action ID=\"Spin\" "
              "server_name=\"spin\" server_timeout=\"15.0\" "
              "spin_dist=\"1.57\" time_allowance=\"10.0\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearLocalCostmap-AfterSpin90R\" "
         "server_timeout=\"10.0\" "
         "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearGlobalCostmap-AfterSpin90R\" "
         "server_timeout=\"10.0\" "
         "service_name=\"global_costmap/clear_entirely_global_costmap\"/>\n";
  xml_file << "                            </Sequence>\n";
  // Stage 5: Spin left 90deg + clear all
  xml_file << "                            <Sequence "
              "name=\"SpinLeftAndClearAll\">\n";
  xml_file << "                                <Action ID=\"Spin\" "
              "server_name=\"spin\" server_timeout=\"15.0\" "
              "spin_dist=\"-1.57\" time_allowance=\"10.0\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearLocalCostmap-AfterSpin90L\" "
         "server_timeout=\"10.0\" "
         "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearGlobalCostmap-AfterSpin90L\" "
         "server_timeout=\"10.0\" "
         "service_name=\"global_costmap/clear_entirely_global_costmap\"/>\n";
  xml_file << "                            </Sequence>\n";
  // Stage 6: Wait 5s + full clear
  xml_file
      << "                            <Sequence name=\"WaitAndFullClear\">\n";
  xml_file
      << "                                <Action ID=\"Wait\" "
         "server_name=\"wait\" server_timeout=\"15.0\" wait_duration=\"5\"/>\n";
  xml_file << "                                <Action "
              "ID=\"ClearEntireCostmap\" name=\"ClearLocalCostmap-AfterWait\" "
              "server_timeout=\"10.0\" "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearGlobalCostmap-AfterWait\" "
         "server_timeout=\"10.0\" "
         "service_name=\"global_costmap/clear_entirely_global_costmap\"/>\n";
  xml_file << "                            </Sequence>\n";
  // Stage 7: Spin 180deg + clear all
  xml_file << "                            <Sequence "
              "name=\"LastResortFullReset\">\n";
  xml_file << "                                <Action ID=\"Spin\" "
              "server_name=\"spin\" server_timeout=\"20.0\" "
              "spin_dist=\"3.14\" time_allowance=\"15.0\"/>\n";
  xml_file << "                                <Action "
              "ID=\"ClearEntireCostmap\" name=\"ClearLocalCostmap-LastResort\" "
              "server_timeout=\"10.0\" "
              "service_name=\"local_costmap/clear_entirely_local_costmap\"/>\n";
  xml_file
      << "                                <Action ID=\"ClearEntireCostmap\" "
         "name=\"ClearGlobalCostmap-LastResort\" "
         "server_timeout=\"10.0\" "
         "service_name=\"global_costmap/clear_entirely_global_costmap\"/>\n";
  xml_file << "                            </Sequence>\n";
  xml_file << "                        </Control>\n";
  xml_file << "                    </ReactiveFallback>\n";
  xml_file << "                </Control>\n";
}

// ============================================================================
// Mode Handlers
// ============================================================================

void handle_waypoints_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  response->status = -1;

  stop_behavior_tree();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "Mode: " << request->mode);
  }

  const std::string &xml_path = paths::WAYPOINTS_XML;
  const std::string &yaml_path = paths::WAYPOINTS_YAML;
  rom_interfaces::msg::ConstructYaml message;

  if (!std::filesystem::exists(xml_path)) {
    if (debug_mode_) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                         "waypoints_mode.xml doesn't exists: cancelling ...");
    }
    return;
  }

  std::ofstream xml_file(xml_path, std::ios::trunc);
  if (!xml_file.is_open()) {
    if (debug_mode_) {
      RCLCPP_ERROR(rclcpp::get_logger("xml constructor"),
                   "waypoints_mode.xml file open error!! cancelling ...");
    }
    return;
  }

  xml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml_file << "<root BTCPP_format=\"3\">\n";
  xml_file << "  <BehaviorTree ID=\"MainTree\">\n";
  xml_file << "    <Sequence name=\"NavigationSequence\">\n";
  xml_file << "      \n";

  for (int i = static_cast<int>(request->scene_poses.size()) - 1; i >= 0; --i) {
    message.pose_names.push_back(request->pose_names[i]);
    message.poses.push_back(request->scene_poses[i]);
    write_recovery_subtree_waypoints(xml_file, request->pose_names[i]);
  }

  xml_file << "    </Sequence>\n";
  xml_file << "  </BehaviorTree>\n";

  if (!append_tree_models(xml_file)) {
    return;
  }

  xml_file << "</root>\n";
  xml_file.flush();
  xml_file.close();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "waypoints_mode.xml created successfully!");
  }

  publisher_->publish(message);

  if (write_waypoints_yaml(yaml_path, request->pose_names, request->poses,
                           request->scene_poses, "waypoints_mode")) {
    response->status = 1;
  }
}

void handle_service_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  ROM_DYNAMICS_UNUSED(response);

  stop_behavior_tree();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "Mode: " << request->mode);
  }

  const std::string &xml_path = paths::SERVICE_XML;
  const std::string &yaml_path = paths::SERVICE_YAML;
  rom_interfaces::msg::ConstructYaml message;

  if (!std::filesystem::exists(xml_path)) {
    if (debug_mode_) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                         "service_mode.xml doesn't exists: creating ...");
    }
    return;
  }

  std::ofstream xml_file(xml_path, std::ios::trunc);
  if (!xml_file.is_open()) {
    if (debug_mode_) {
      RCLCPP_ERROR(rclcpp::get_logger("xml constructor"),
                   "service_mode.xml file open error!!");
    }
    return;
  }

  xml_file << "<?xml version=\"1.0\"?>\n";
  xml_file << "<root main_tree_to_execute=\"MainTree\">\n";
  xml_file << "    <BehaviorTree ID=\"MainTree\">\n";
  xml_file << "        <Sequence name=\"NavigationSequence\">\n";

  bool first_time_loop = true;
  for (int i = static_cast<int>(request->scene_poses.size()) - 1; i >= 0; --i) {
    message.pose_names.push_back(request->pose_names[i]);
    message.poses.push_back(request->scene_poses[i]);

    if (!first_time_loop) {
      xml_file << "            <Delay delay_msec=\"15000\">\n";
    }

    write_recovery_subtree_action_format(xml_file, request->pose_names[i]);

    if (!first_time_loop) {
      xml_file << "            </Delay>\n";
    }
    first_time_loop = false;
  }

  xml_file << "            </Sequence>\n";
  xml_file << "    </BehaviorTree>\n";

  if (!append_tree_models(xml_file)) {
    return;
  }

  xml_file << "</root>\n";
  xml_file.flush();
  xml_file.close();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "service_mode.xml created successfully!");
  }

  write_waypoints_yaml(yaml_path, request->pose_names, request->poses,
                       request->scene_poses, "service_mode");
}

void handle_patrol_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  ROM_DYNAMICS_UNUSED(response);

  stop_behavior_tree();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "Mode: " << request->mode);
  }

  const std::string &xml_path = paths::PATROL_XML;
  const std::string &yaml_path = paths::PATROL_YAML;
  rom_interfaces::msg::ConstructYaml message;

  if (!std::filesystem::exists(xml_path)) {
    if (debug_mode_) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                         "patrol_mode.xml doesn't exists: creating ...");
    }
    return;
  }

  std::ofstream xml_file(xml_path, std::ios::trunc);
  if (!xml_file.is_open()) {
    if (debug_mode_) {
      RCLCPP_ERROR(rclcpp::get_logger("xml constructor"),
                   "patrol_mode.xml file open error!!");
    }
    return;
  }

  xml_file << "<?xml version=\"1.0\"?>\n";
  xml_file << "<root main_tree_to_execute=\"MainTree\">\n";
  xml_file << "    <BehaviorTree ID=\"MainTree\">\n";
  xml_file << "        <Repeat num_cycles=\"100\">\n";
  xml_file << "            <Sequence name=\"NavigationSequence\">\n";

  for (int i = static_cast<int>(request->scene_poses.size()) - 1; i >= 0; --i) {
    message.pose_names.push_back(request->pose_names[i]);
    message.poses.push_back(request->scene_poses[i]);
    write_recovery_subtree_action_format(xml_file, request->pose_names[i]);
  }

  xml_file << "            </Sequence>\n";
  xml_file << "        </Repeat>\n";
  xml_file << "    </BehaviorTree>\n";

  if (!append_tree_models(xml_file)) {
    return;
  }

  xml_file << "</root>\n";
  xml_file.flush();
  xml_file.close();

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "patrol_mode.xml created successfully!");
  }

  write_waypoints_yaml(yaml_path, request->pose_names, request->poses,
                       request->scene_poses, "patrol_mode");
}

void handle_goal_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request>
        request) {
  stop_behavior_tree();

  std::string cmd2 = request->command;
  std::thread([cmd2]() {
    int ret = system(cmd2.c_str());
    if (ret == -1) {
      if (debug_mode_) {
        RCLCPP_ERROR(rclcpp::get_logger("construct_xml"),
                     "Failed to bt tree: %s", cmd2.c_str());
      }
    } else {
      if (debug_mode_) {
        RCLCPP_INFO(rclcpp::get_logger("construct_xml"),
                    "Run BT tree successfully and Done %s", cmd2.c_str());
      }
    }
  }).detach();
}

void handle_eraser_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request>
        request) {
  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "Mode: " << request->mode);
  }

  const std::vector<std::pair<std::string, std::string>> files_to_erase = {
      {paths::WAYPOINTS_XML, "waypoints_mode.xml"},
      {paths::WAYPOINTS_YAML, "waypoints_mode.yaml"},
      {paths::SERVICE_XML, "services_mode.xml"},
      {paths::SERVICE_YAML, "services_mode.yaml"},
      {paths::PATROL_XML, "patrols_mode.xml"},
      {paths::PATROL_YAML, "patrols_mode.yaml"}};

  for (const auto &item : files_to_erase) {
    std::ofstream ofs(item.first, std::ios::trunc);
    if (!ofs.is_open()) {
      if (debug_mode_) {
        RCLCPP_ERROR(rclcpp::get_logger("xml constructor"),
                     "%s file delete error!!", item.second.c_str());
      }
      return;
    }
    ofs << "hello";
    ofs.flush();
    ofs.close();
  }
}

void handle_path_mode(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  response->status = -1;

  if (debug_mode_) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger("xml constructor"),
                       "Mode: " << request->mode);
  }

  if (write_waypoints_yaml(paths::PATH_YAML, request->pose_names,
                           request->poses, request->scene_poses, "path_mode")) {
    response->status = 1;
  }
}

// ============================================================================
// Service Callback Dispatcher
// ============================================================================
void construct_xml_file(
    const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
    std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response) {
  const std::string &request_mode = request->mode;

  if (request_mode == "waypoints_mode") {
    handle_waypoints_mode(request, response);
  } else if (request_mode == "service_mode") {
    handle_service_mode(request, response);
  } else if (request_mode == "patrol_mode") {
    handle_patrol_mode(request, response);
  } else if (request_mode == "goal_mode") {
    handle_goal_mode(request);
  } else if (request_mode == "eraser_mode") {
    handle_eraser_mode(request);
  } else if (request_mode == "path_mode") {
    handle_path_mode(request, response);
  } else if (request_mode == "get_wp_list") {
    parse_waypoints_yaml(paths::WAYPOINTS_YAML, "get_wp_list", response);
  } else if (request_mode == "get_srv_list") {
    parse_waypoints_yaml(paths::SERVICE_YAML, "get_srv_list", response);
  } else if (request_mode == "get_patrol_list") {
    parse_waypoints_yaml(paths::PATROL_YAML, "get_patrol_list", response);
  } else if (request_mode == "get_path_list") {
    parse_waypoints_yaml(paths::PATH_YAML, "get_path_list", response);
  } else {
    if (debug_mode_) {
      RCLCPP_INFO(rclcpp::get_logger("construct_xml"), "Invalid mode");
    }
  }
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char **argv) {
  rclcpp::init(argc, argv);

  const char *debug_env = std::getenv("ROM_DYNAMICS_DEBUG");
  debug_mode_ = (debug_env != nullptr && std::string(debug_env) == "true");
  RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "debug_mode_: %s",
              debug_mode_ ? "true" : "false");

  // NodeOptions ဆောက်ခြင်း (CLI use_sim_time / launch parameters များအတွက်)
  auto node_options = rclcpp::NodeOptions();

  std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared(
      "construct_bt_xml_server", rom_robot_namespace, node_options);
  bt_stop_node = rclcpp::Node::make_shared("bt_stop_client_node",
                                           rom_robot_namespace, node_options);

  auto qos = rclcpp::QoS(1);
  qos.transient_local();

  // အခြား qt app များမှ waypoints များကို ရယူရန် အတွက် publisher တည်ဆောက်ခြင်း
  publisher_ = node->create_publisher<rom_interfaces::msg::ConstructYaml>(
      "waypoints_list", qos);

  // qt မှ wp များကို behavior tree တည်ဆောက်ပေးရန် အတွက် service တည်ဆောက်ခြင်း
  rclcpp::Service<rom_interfaces::srv::ConstructYaml>::SharedPtr service =
      node->create_service<rom_interfaces::srv::ConstructYaml>(
          "construct_yaml_and_bt", &construct_xml_file);

  bt_client = bt_stop_node->create_client<std_srvs::srv::SetBool>("bt_stop");

  if (debug_mode_) {
    RCLCPP_INFO(rclcpp::get_logger("construct_xml"),
                "Ready to construct behavior tree and yaml for waypoints, "
                "service and patrol mode");
  }

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
