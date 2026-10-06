#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rom_interfaces/msg/construct_yaml.hpp"
#include "rom_interfaces/srv/construct_yaml.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "yaml-cpp/yaml.h"

namespace
{
using ConstructYaml = rom_interfaces::srv::ConstructYaml;
using ConstructYamlMessage = rom_interfaces::msg::ConstructYaml;
using Pose = geometry_msgs::msg::Pose;
using PoseStamped = geometry_msgs::msg::PoseStamped;

constexpr char kDataRoot[] = "/home/buc_robot/data";
constexpr char kTreeModelPath[] = "/home/buc_robot/data/trees/tree_nodes_models.xml";
constexpr char kMapFrame[] = "map";

rclcpp::Publisher<ConstructYamlMessage>::SharedPtr waypoint_publisher;
rclcpp::Client<std_srvs::srv::SetBool>::SharedPtr bt_stop_client;
rclcpp::Node::SharedPtr bt_stop_node;
bool debug_enabled = false;

const rclcpp::Logger logger()
{
  return rclcpp::get_logger("construct_xml_server_bt");
}

void debug_info(const std::string &message)
{
  if (debug_enabled) {
    RCLCPP_INFO_STREAM(logger(), message);
  }
}

std::string robot_namespace()
{
  const char *value = std::getenv("ROM_ROBOT_NAMESPACE");
  return value == nullptr ? "" : value;
}

std::string nav2_name(const std::string &name)
{
  return "/" + robot_namespace() + "/" + name;
}

bool stop_behavior_tree()
{
  if (!bt_stop_client || !bt_stop_node) {
    RCLCPP_ERROR(logger(), "Behavior tree stop client is not initialized");
    return false;
  }

  if (!bt_stop_client->wait_for_service(std::chrono::seconds(3))) {
    RCLCPP_WARN(logger(), "Service 'bt_stop' is unavailable");
    return false;
  }

  auto request = std::make_shared<std_srvs::srv::SetBool::Request>();
  request->data = true;
  auto future = bt_stop_client->async_send_request(request);
  const auto result = rclcpp::spin_until_future_complete(
    bt_stop_node, future, std::chrono::seconds(5));

  if (result != rclcpp::FutureReturnCode::SUCCESS) {
    RCLCPP_ERROR(logger(), "Timed out while stopping the behavior tree");
    return false;
  }

  const auto response = future.get();
  if (!response->success) {
    RCLCPP_WARN(logger(), "Behavior tree did not stop: %s", response->message.c_str());
  }
  return response->success;
}

bool open_for_rewrite(const std::string &path, std::ofstream &file)
{
  if (!std::filesystem::exists(path)) {
    RCLCPP_ERROR(logger(), "Required file does not exist: %s", path.c_str());
    return false;
  }

  file.open(path, std::ios::trunc);
  if (!file.is_open()) {
    RCLCPP_ERROR(logger(), "Cannot open file for writing: %s", path.c_str());
    return false;
  }
  return true;
}

bool append_tree_model(std::ofstream &xml)
{
  std::ifstream model(kTreeModelPath);
  if (!model.is_open()) {
    RCLCPP_ERROR(logger(), "Cannot open tree model: %s", kTreeModelPath);
    return false;
  }
  xml << model.rdbuf();
  return static_cast<bool>(xml);
}

void write_pose_yaml(YAML::Emitter &out, const PoseStamped &pose, const Pose &scene_pose,
  const std::string &name)
{
  out << YAML::BeginMap << YAML::Key << "name" << YAML::Value << name;
  out << YAML::Key << "frame_id" << YAML::Value << kMapFrame;
  out << YAML::Key << "pose" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "position" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "x" << YAML::Value << pose.pose.position.x;
  out << YAML::Key << "y" << YAML::Value << pose.pose.position.y;
  out << YAML::Key << "z" << YAML::Value << pose.pose.position.z << YAML::EndMap;
  out << YAML::Key << "orientation" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "x" << YAML::Value << pose.pose.orientation.x;
  out << YAML::Key << "y" << YAML::Value << pose.pose.orientation.y;
  out << YAML::Key << "z" << YAML::Value << pose.pose.orientation.z;
  out << YAML::Key << "w" << YAML::Value << pose.pose.orientation.w << YAML::EndMap;
  out << YAML::EndMap;
  out << YAML::Key << "scene_poses" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "x" << YAML::Value << scene_pose.position.x;
  out << YAML::Key << "y" << YAML::Value << scene_pose.position.y;
  out << YAML::Key << "phi" << YAML::Value << scene_pose.orientation.w;
  out << YAML::EndMap << YAML::EndMap;
}

bool write_waypoint_yaml(const std::string &path,
  const ConstructYaml::Request &request)
{
  std::ofstream file;
  if (!open_for_rewrite(path, file)) {
    return false;
  }

  YAML::Emitter out;
  out << YAML::BeginMap << YAML::Key << "waypoints" << YAML::Value << YAML::BeginSeq;
  const size_t count = std::min(request.pose_names.size(),
    std::min(request.poses.size(), request.scene_poses.size()));
  for (size_t offset = 0; offset < count; ++offset) {
    const size_t index = count - offset - 1;
    write_pose_yaml(out, request.poses[index], request.scene_poses[index],
      request.pose_names[index]);
  }
  out << YAML::EndSeq << YAML::EndMap;
  file << out.c_str() << '\n';
  const bool success = static_cast<bool>(file);
  if (success) {
    debug_info("Waypoint YAML created successfully: " + path);
  }
  return success;
}

bool read_waypoint_yaml(const std::string &path, ConstructYaml::Response &response)
{
  try {
    const YAML::Node root = YAML::LoadFile(path);
    const YAML::Node waypoints = root["waypoints"];
    if (!waypoints || !waypoints.IsSequence()) {
      RCLCPP_ERROR(logger(), "Invalid waypoint YAML: %s", path.c_str());
      return false;
    }

    for (const auto &item : waypoints) {
      response.pose_names.push_back(item["name"].as<std::string>());
      PoseStamped pose;
      pose.header.frame_id = item["frame_id"] ? item["frame_id"].as<std::string>() : kMapFrame;
      const auto position = item["pose"]["position"];
      const auto orientation = item["pose"]["orientation"];
      pose.pose.position.x = position["x"].as<double>();
      pose.pose.position.y = position["y"].as<double>();
      pose.pose.position.z = position["z"].as<double>();
      pose.pose.orientation.x = orientation["x"].as<double>();
      pose.pose.orientation.y = orientation["y"].as<double>();
      pose.pose.orientation.z = orientation["z"].as<double>();
      pose.pose.orientation.w = orientation["w"].as<double>();
      response.poses.push_back(pose);

      Pose scene_pose;
      scene_pose.position.x = item["scene_poses"]["x"].as<double>();
      scene_pose.position.y = item["scene_poses"]["y"].as<double>();
      scene_pose.orientation.w = item["scene_poses"]["phi"].as<double>();
      response.scene_poses.push_back(scene_pose);
    }
  } catch (const YAML::Exception &error) {
    RCLCPP_ERROR(logger(), "Cannot read %s: %s", path.c_str(), error.what());
    return false;
  }
  return true;
}

void append_waypoint_message(const ConstructYaml::Request &request,
  ConstructYamlMessage &message)
{
  const size_t count = std::min(request.pose_names.size(), request.scene_poses.size());
  for (size_t offset = 0; offset < count; ++offset) {
    const size_t index = count - offset - 1;
    message.pose_names.push_back(request.pose_names[index]);
    message.poses.push_back(request.scene_poses[index]);
  }
}

void write_modern_navigation_node(std::ofstream &xml_file, const std::string &goal)
{
  const std::string ns_prefix = "/" + robot_namespace() + "/";

  xml_file << "      <RecoveryNode name=\"NavigateRecovery\"\n";
  xml_file << "                    number_of_retries=\"-1\">\n";
  xml_file << "        <PipelineSequence name=\"NavigateWithReplanning\">\n";
  xml_file << "          <RateController hz=\"1.0\">\n";
  xml_file << "            <RecoveryNode name=\"ComputePathToPose\"\n";
  xml_file << "                          number_of_retries=\"1\">\n";
  xml_file << "              <ComputePathToPose goal=\"{";
  xml_file << goal << "}\"\n";
  xml_file << "                                 start=\"\"\n";
  xml_file << "                                 planner_id=\"GridBased\"\n";
  xml_file << "                                 server_name=\"" << ns_prefix << "compute_path_to_pose\"\n";
  xml_file << "                                 server_timeout=\"1000.0\"\n";
  xml_file << "                                 path=\"{path}\"/>\n";
  xml_file << "              <ClearEntireCostmap name=\"ClearGlobalCostmap-Context\"\n";
  xml_file << "                                  service_name=\"" << ns_prefix << "global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"1000.0\"/>\n";
  xml_file << "            </RecoveryNode>\n";
  xml_file << "          </RateController>\n";
  xml_file << "          <RecoveryNode name=\"FollowPath\"\n";
  xml_file << "                        number_of_retries=\"1\">\n";
  xml_file << "            <FollowPath controller_id=\"FollowPath\"\n";
  xml_file << "                        path=\"{path}\"\n";
  xml_file << "                        goal_checker_id=\"\"\n";
  xml_file << "                        server_name=\"" << ns_prefix << "follow_path\"\n";
  xml_file << "                        server_timeout=\"10.0\"/>\n";
  xml_file << "            <ClearEntireCostmap name=\"ClearLocalCostmap-Context\"\n";
  xml_file << "                                service_name=\"" << ns_prefix << "local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                server_timeout=\"1000.0\"/>\n";
  xml_file << "          </RecoveryNode>\n";
  xml_file << "        </PipelineSequence>\n";
  xml_file << "        <ReactiveFallback name=\"RecoveryFallback\">\n";
  xml_file << "          <GoalUpdated/>\n";
  xml_file << "          <RoundRobin name=\"RecoveryActions\">\n";
  xml_file << "            <Sequence name=\"WaitClearAndReplan\">\n";
  xml_file << "              <Wait wait_duration=\"5\"\n";
  xml_file << "                    server_name=\"" << ns_prefix << "wait\"\n";
  xml_file << "                    server_timeout=\"10.0\"/>\n";
  xml_file << "              <ClearEntireCostmap name=\"ClearLocalCostmap-Subtree\"\n";
  xml_file << "                                  service_name=\"" << ns_prefix << "local_costmap/clear_entirely_local_costmap\"\n";
  xml_file << "                                  server_timeout=\"1000.0\"/>\n";
  xml_file << "              <ClearEntireCostmap name=\"ClearGlobalCostmap-Subtree\"\n";
  xml_file << "                                  service_name=\"" << ns_prefix << "global_costmap/clear_entirely_global_costmap\"\n";
  xml_file << "                                  server_timeout=\"1000.0\"/>\n";
  xml_file << "            </Sequence>\n";
  xml_file << "            <Spin spin_dist=\"0.35\"\n";
  xml_file << "                  time_allowance=\"10.0\"\n";
  xml_file << "                  server_name=\"" << ns_prefix << "spin\"\n";
  xml_file << "                  server_timeout=\"10.0\"/>\n";
  xml_file << "          </RoundRobin>\n";
  xml_file << "        </ReactiveFallback>\n";
  xml_file << "      </RecoveryNode>\n";
}

bool write_waypoints_xml(const std::string &path, const ConstructYaml::Request &request,
  ConstructYamlMessage &message)
{
  std::ofstream xml_file;
  if (!open_for_rewrite(path, xml_file)) {
    return false;
  }
  xml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml_file << "<root BTCPP_format=\"3\">\n";
  xml_file << "  <BehaviorTree ID=\"MainTree\">\n";
  xml_file << "    <Sequence name=\"NavigationSequence\">\n";

  const size_t count = std::min(request.pose_names.size(), request.poses.size());
  for (size_t offset = 0; offset < count; ++offset) {
    const size_t index = count - offset - 1;
    write_modern_navigation_node(xml_file, request.pose_names[index]);
  }
  xml_file << "    </Sequence>\n";
  xml_file << "  </BehaviorTree>\n";
  if (!append_tree_model(xml_file)) {
    return false;
  }
  xml_file << "</root>\n";
  append_waypoint_message(request, message);
  const bool success = static_cast<bool>(xml_file);
  if (success) {
    debug_info("waypoints_mode.xml created successfully!");
  }
  return success;
}

void write_legacy_compute_path(std::ofstream &xml_file, const std::string &prefix,
  const std::string &goal)
{
  xml_file << prefix << "<Decorator ID=\"RateController\" hz=\"1.0\">\n";
  xml_file << prefix << "    <Control ID=\"RecoveryNode\"\n";
  xml_file << prefix << "                          name=\"ComputePathToPose\"\n";
  xml_file << prefix << "                          number_of_retries=\"1\">\n";
  xml_file << prefix << "        <Action ID=\"ComputePathToPose\" goal=\"{";
  xml_file << goal << "}\"\n";
  xml_file << prefix << "                                  path=\"{path}\"\n";
  xml_file << prefix << "                                  planner_id=\"GridBased\"\n";
  xml_file << prefix << "                                  server_name=\""
           << nav2_name("compute_path_to_pose") << "\"\n";
  xml_file << prefix << "                                  server_timeout=\"10.0\"/>\n";
  xml_file << prefix << "        <Action ID=\"ClearEntireCostmap\"\n";
  xml_file << prefix << "                name=\"ClearGlobalCostmap-Context\"\n";
  xml_file << prefix << "                service_name=\""
           << nav2_name("global_costmap/clear_entirely_global_costmap") << "\"\n";
  xml_file << prefix << "                server_timeout=\"1000.0\"/>\n";
  xml_file << prefix << "    </Control>\n";
  xml_file << prefix << "</Decorator>\n";
}

void write_legacy_follow_path(std::ofstream &xml_file, const std::string &prefix)
{
  xml_file << prefix << "<Control ID=\"RecoveryNode\"\n";
  xml_file << prefix << "                       name=\"FollowPath\"\n";
  xml_file << prefix << "                       number_of_retries=\"1\">\n";
  xml_file << prefix << "    <Action ID=\"FollowPath\"\n";
  xml_file << prefix << "            controller_id=\"FollowPath\"\n";
  xml_file << prefix << "            path=\"{path}\"\n";
  xml_file << prefix << "            server_name=\"" << nav2_name("follow_path") << "\"\n";
  xml_file << prefix << "            server_timeout=\"10.0\"/>\n";
  xml_file << prefix << "    <Action ID=\"ClearEntireCostmap\"\n";
  xml_file << prefix << "            name=\"ClearLocalCostmap-Context\"\n";
  xml_file << prefix << "            service_name=\""
           << nav2_name("local_costmap/clear_entirely_local_costmap") << "\"\n";
  xml_file << prefix << "            server_timeout=\"1000.0\"/>\n";
  xml_file << prefix << "</Control>\n";
}

void write_legacy_recovery(std::ofstream &xml_file, const std::string &prefix)
{
  const std::string recovery = prefix + "    ";
  const std::string actions = recovery + "    ";
  const std::string action = actions + "    ";

  xml_file << prefix << "<ReactiveFallback name=\"RecoveryFallback\">\n";
  xml_file << recovery << "<Condition ID=\"GoalUpdated\"/>\n";
  xml_file << recovery << "<Control ID=\"RoundRobin\"\n";
  xml_file << recovery << "                      name=\"RecoveryActions\">\n";
  xml_file << actions << "<Sequence name=\"WaitClearAndReplan\">\n";
  xml_file << action << "<Action ID=\"Wait\"\n";
  xml_file << action << "        server_name=\"" << nav2_name("wait") << "\"\n";
  xml_file << action << "        server_timeout=\"10.0\"\n";
  xml_file << action << "        wait_duration=\"5\"/>\n";
  xml_file << action << "<Action ID=\"ClearEntireCostmap\"\n";
  xml_file << action << "        name=\"ClearLocalCostmap-Subtree\"\n";
  xml_file << action << "        service_name=\""
           << nav2_name("local_costmap/clear_entirely_local_costmap") << "\"\n";
  xml_file << action << "        server_timeout=\"1000.0\"/>\n";
  xml_file << action << "<Action ID=\"ClearEntireCostmap\"\n";
  xml_file << action << "        name=\"ClearGlobalCostmap-Subtree\"\n";
  xml_file << action << "        service_name=\""
           << nav2_name("global_costmap/clear_entirely_global_costmap") << "\"\n";
  xml_file << action << "        server_timeout=\"1000.0\"/>\n";
  xml_file << actions << "</Sequence>\n";
  xml_file << actions << "<Action ID=\"Spin\"\n";
  xml_file << actions << "        server_name=\"" << nav2_name("spin") << "\"\n";
  xml_file << actions << "        server_timeout=\"10.0\"\n";
  xml_file << actions << "        spin_dist=\"0.35\"\n";
  xml_file << actions << "        time_allowance=\"10.0\"/>\n";
  xml_file << recovery << "</Control>\n";
  xml_file << prefix << "</ReactiveFallback>\n";
}

void write_legacy_navigation_node(std::ofstream &xml_file, const std::string &goal,
  bool add_delay, bool patrol)
{
  const std::string indent = patrol ? "                " : "            ";
  if (add_delay) {
    xml_file << "            <Delay delay_msec=\"15000\">\n";
  }

  xml_file << indent << "<Control ID=\"RecoveryNode\"\n";
  xml_file << indent << "                       name=\"NavigateRecovery\"\n";
  xml_file << indent << "                       number_of_retries=\"-1\">\n";
  xml_file << indent << "    <Control ID=\"PipelineSequence\"\n";
  xml_file << indent << "                            name=\"NavigateWithReplanning\">\n";
  write_legacy_compute_path(xml_file, indent + "        ", goal);
  write_legacy_follow_path(xml_file, indent + "        ");
  xml_file << indent << "    </Control>\n";
  write_legacy_recovery(xml_file, indent + "    ");
  xml_file << indent << "</Control>\n";

  if (add_delay) {
    xml_file << "            </Delay>\n";
  }
}

bool write_service_or_patrol_xml(const std::string &path, const ConstructYaml::Request &request,
  ConstructYamlMessage &message, bool patrol)
{
  std::ofstream xml_file;
  if (!open_for_rewrite(path, xml_file)) {
    return false;
  }
  xml_file << "<?xml version=\"1.0\"?>\n";
  xml_file << "<root main_tree_to_execute=\"MainTree\">\n";
  xml_file << "  <BehaviorTree ID=\"MainTree\">\n";
  if (patrol) {
    xml_file << "    <Repeat num_cycles=\"100\">\n";
    xml_file << "      <Sequence name=\"NavigationSequence\">\n";
  } else {
    xml_file << "    <Sequence name=\"NavigationSequence\">\n";
  }

  const size_t count = std::min(request.pose_names.size(), request.poses.size());
  for (size_t offset = 0; offset < count; ++offset) {
    const size_t index = count - offset - 1;
    write_legacy_navigation_node(xml_file, request.pose_names[index], !patrol && offset > 0, patrol);
    message.pose_names.push_back(request.pose_names[index]);
    message.poses.push_back(request.scene_poses[index]);
  }
  if (patrol) {
    xml_file << "      </Sequence>\n";
    xml_file << "    </Repeat>\n";
  } else {
    xml_file << "    </Sequence>\n";
  }
  xml_file << "  </BehaviorTree>\n";
  if (!append_tree_model(xml_file)) {
    return false;
  }
  xml_file << "</root>\n";
  const bool success = static_cast<bool>(xml_file);
  if (success) {
    debug_info(std::string(patrol ? "patrol_mode.xml" : "service_mode.xml") +
      " created successfully!");
  }
  return success;
}

bool clear_file(const std::string &path)
{
  std::ofstream file(path, std::ios::trunc);
  if (!file.is_open()) {
    RCLCPP_ERROR(logger(), "Cannot clear file: %s", path.c_str());
    return false;
  }
  return true;
}

void construct_xml_callback(const std::shared_ptr<ConstructYaml::Request> request,
  std::shared_ptr<ConstructYaml::Response> response)
{
  response->status = -1;
  const std::string &mode = request->mode;
  debug_info("Mode: " + mode);

  if (mode == "goal_mode") {
    stop_behavior_tree();
    const std::string command = request->command;
    std::thread([command]() {
      const int result = std::system(command.c_str());
      if (result != 0) {
        RCLCPP_ERROR(logger(), "Behavior tree command failed: %s", command.c_str());
      }
    }).detach();
    debug_info("Started behavior tree command: " + command);
    return;
  }

  if (mode == "eraser_mode") {
    const std::vector<std::string> files = {
      std::string(kDataRoot) + "/trees/waypoints_mode.xml",
      std::string(kDataRoot) + "/waypoints/waypoints_mode.yaml",
      std::string(kDataRoot) + "/trees/service_mode.xml",
      std::string(kDataRoot) + "/waypoints/service_mode.yaml",
      std::string(kDataRoot) + "/trees/patrol_mode.xml",
      std::string(kDataRoot) + "/waypoints/patrol_mode.yaml"};
    bool success = true;
    for (const auto &file : files) success = clear_file(file) && success;
    response->status = success ? 1 : -1;
    debug_info(success ? "All mode files cleared successfully." : "Failed to clear one or more mode files.");
    return;
  }

  if (mode == "path_mode") {
    const bool success = write_waypoint_yaml(
      std::string(kDataRoot) + "/waypoints/path_mode.yaml", *request);
    response->status = success ? 1 : -1;
    debug_info(success ? "path_mode.yaml created successfully!" : "Failed to create path_mode.yaml.");
    return;
  }

  const bool is_waypoints = mode == "waypoints_mode";
  const bool is_service = mode == "service_mode";
  const bool is_patrol = mode == "patrol_mode";
  if (is_waypoints || is_service || is_patrol) {
    stop_behavior_tree();
    ConstructYamlMessage message;
    bool success = false;
    if (is_waypoints) {
      success = write_waypoints_xml(std::string(kDataRoot) + "/trees/waypoints_mode.xml", *request, message) &&
        write_waypoint_yaml(std::string(kDataRoot) + "/waypoints/waypoints_mode.yaml", *request);
    } else if (is_service) {
      success = write_service_or_patrol_xml(std::string(kDataRoot) + "/trees/service_mode.xml", *request, message, false) &&
        write_waypoint_yaml(std::string(kDataRoot) + "/waypoints/service_mode.yaml", *request);
    } else {
      success = write_service_or_patrol_xml(std::string(kDataRoot) + "/trees/patrol_mode.xml", *request, message, true) &&
        write_waypoint_yaml(std::string(kDataRoot) + "/waypoints/patrol_mode.yaml", *request);
    }
    if (success && is_waypoints && waypoint_publisher) waypoint_publisher->publish(message);
    response->status = success ? 1 : -1;
    debug_info(success ? "XML and YAML files created successfully for mode: " + mode :
      "Failed to create XML/YAML files for mode: " + mode);
    return;
  }

  const std::vector<std::pair<std::string, std::string>> list_modes = {
    {"get_wp_list", "waypoints_mode"}, {"get_srv_list", "service_mode"},
    {"get_patrol_list", "patrol_mode"}, {"get_path_list", "path_mode"}};
  for (const auto &[list_mode, file_mode] : list_modes) {
    if (mode == list_mode) {
      const std::string path = std::string(kDataRoot) + "/waypoints/" + file_mode + ".yaml";
      response->status = read_waypoint_yaml(path, *response) ? 1 : -1;
      debug_info("Parsed " + std::to_string(response->pose_names.size()) +
        " waypoints for mode: " + mode);
      return;
    }
  }

  RCLCPP_WARN(logger(), "Unknown construct mode: %s", mode.c_str());
}

void stop_robot_callback(const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
  std::shared_ptr<std_srvs::srv::SetBool::Response> response)
{
  if (!request->data) {
    response->success = false;
    response->message = "Set data to true to stop the robot.";
    return;
  }
  response->success = stop_behavior_tree();
  response->message = response->success ? "Behavior tree stopped." : "Failed to stop behavior tree.";
}
}  // namespace

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  const char *debug = std::getenv("ROM_DYNAMICS_DEBUG");
  debug_enabled = debug != nullptr && std::string(debug) == "true";
  (void)debug_enabled;

  const auto options = rclcpp::NodeOptions();
  auto node = rclcpp::Node::make_shared("construct_bt_xml_server", robot_namespace(), options);
  bt_stop_node = rclcpp::Node::make_shared("bt_stop_client_node", robot_namespace(), options);

  auto qos = rclcpp::QoS(1);
  qos.transient_local();
  waypoint_publisher = node->create_publisher<ConstructYamlMessage>("waypoints_list", qos);
  bt_stop_client = bt_stop_node->create_client<std_srvs::srv::SetBool>("bt_stop");
  auto construct_service = node->create_service<ConstructYaml>("construct_yaml_and_bt", &construct_xml_callback);
  auto stop_service = node->create_service<std_srvs::srv::SetBool>("stop_robot", &stop_robot_callback);
  (void)construct_service;
  (void)stop_service;

  RCLCPP_INFO(logger(), "Ready to construct behavior-tree XML and waypoint YAML files");
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
