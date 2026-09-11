#include "nav2_behavior_tree/behavior_tree_engine.hpp"
#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp_v3/blackboard.h"
#include "ament_index_cpp/get_package_share_directory.hpp"
#include <geometry_msgs/msg/pose_stamped.hpp>
#include "behaviortree_cpp_v3/loggers/bt_zmq_publisher.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <std_srvs/srv/set_bool.hpp>

const std::string bt_xml_file = "/home/buc_robot/data/trees/waypoints_mode.xml";
const std::string waypoints_yaml_file = "/home/buc_robot/data/waypoints/waypoints_mode.yaml";
const std::string rom_robot_namespace = std::getenv("ROM_ROBOT_NAMESPACE"); 

/* std::atomic<bool> ensures that updates are immediately visible to all threads. */
#include <atomic>
std::atomic<bool> stop_requested_ = false;
/* ဒါပေမဲ့ နောက်ပြသနာတခုက service callback() က bt tree ပြီးအောင်စောင့်နေလို့ async မဖြစ်။ */

void stopServiceCallback(const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
                             std::shared_ptr<std_srvs::srv::SetBool::Response> response)
{
    if (request->data)
    {
        stop_requested_ = true;

        RCLCPP_WARN(rclcpp::get_logger("Main"), "Stopping behavior tree...");
        response->success = true;
        response->message = "Behavior tree stopping...";
    }
    else
    {
        RCLCPP_INFO(rclcpp::get_logger("Main"), "Received stop request but ignored.");
        response->success = false;
        response->message = "Stop request ignored.";
    }
}

void load_yaml_file(const std::string& file_path, std::shared_ptr<BT::Blackboard> blackboard)
{
    try {
        YAML::Node config = YAML::LoadFile(file_path);
        if (config["waypoints"]) 
        {
            for (const auto& wp : config["waypoints"]) 
            {
                geometry_msgs::msg::PoseStamped pose;
                std::string goal_pose_name = wp["name"].as<std::string>();

                if (wp["frame_id"]) 
                {
                    pose.header.frame_id = wp["frame_id"].as<std::string>();
                } else {
                    pose.header.frame_id = "map"; 
                }

                if (wp["pose"]) {
                    if (wp["pose"]["position"]) {
                        pose.pose.position.x = wp["pose"]["position"]["x"].as<double>();
                        pose.pose.position.y = wp["pose"]["position"]["y"].as<double>();
                        pose.pose.position.z = wp["pose"]["position"]["z"].as<double>();
                    }
                    if (wp["pose"]["orientation"]) {
                        pose.pose.orientation.x = wp["pose"]["orientation"]["x"].as<double>();
                        pose.pose.orientation.y = wp["pose"]["orientation"]["y"].as<double>();
                        pose.pose.orientation.z = wp["pose"]["orientation"]["z"].as<double>();
                        pose.pose.orientation.w = wp["pose"]["orientation"]["w"].as<double>();
                    }
                }
                
                blackboard->set(goal_pose_name, pose);

                RCLCPP_INFO(rclcpp::get_logger("Main"), "Loaded waypoint: %s", goal_pose_name.c_str());
            }
        } else {
            RCLCPP_WARN(rclcpp::get_logger("Main"), "No 'waypoints' key found in YAML file: %s", file_path.c_str());
            // should return or not ?
        }
    } catch (const YAML::BadFile& e) {
        RCLCPP_ERROR(rclcpp::get_logger("Main"), "Error loading YAML file '%s': %s", file_path.c_str(), e.what());
    } catch (const YAML::ParserException& e) {
        RCLCPP_ERROR(rclcpp::get_logger("Main"), "YAML parsing error in '%s': %s", file_path.c_str(), e.what());
    }
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    
    auto node = rclcpp::Node::make_shared("autonomy_node", rom_robot_namespace);
    auto stop_service_node = rclcpp::Node::make_shared("stop_service_node", rom_robot_namespace);
    
    rclcpp::executors::MultiThreadedExecutor executor;

    auto stop_service_ = stop_service_node->create_service<std_srvs::srv::SetBool>(
        "/edu_robot/bt_stop", stopServiceCallback);
    executor.add_node(stop_service_node);
    executor.add_node(node);
    
    std::vector<std::string> plugins = 
    {
        "nav2_assisted_teleop_action_bt_node",
        "nav2_assisted_teleop_cancel_bt_node",
        "nav2_back_up_action_bt_node",
        "nav2_back_up_cancel_bt_node",
        "nav2_clear_costmap_service_bt_node",
        "nav2_compute_path_through_poses_action_bt_node",
        "nav2_compute_path_to_pose_action_bt_node",
        "nav2_controller_cancel_bt_node",
        "nav2_controller_selector_bt_node",
        "nav2_distance_controller_bt_node",
        "nav2_distance_traveled_condition_bt_node",
        "nav2_drive_on_heading_bt_node",
        "nav2_drive_on_heading_cancel_bt_node",
        "nav2_follow_path_action_bt_node",
        "nav2_get_pose_from_path_action_bt_node",
        "nav2_globally_updated_goal_condition_bt_node",
        "nav2_goal_checker_selector_bt_node",
        "nav2_goal_reached_condition_bt_node",
        "nav2_goal_updated_condition_bt_node",
        "nav2_goal_updated_controller_bt_node",
        "nav2_goal_updater_node_bt_node",
        "nav2_initial_pose_received_condition_bt_node",
        "nav2_is_battery_charging_condition_bt_node",
        "nav2_is_battery_low_condition_bt_node",
        "nav2_is_path_valid_condition_bt_node",
        "nav2_is_stuck_condition_bt_node",
        "nav2_navigate_through_poses_action_bt_node",
        "nav2_navigate_to_pose_action_bt_node",
        "nav2_path_expiring_timer_condition",
        "nav2_path_longer_on_approach_bt_node",
        "nav2_pipeline_sequence_bt_node",
        "nav2_planner_selector_bt_node",
        "nav2_progress_checker_selector_bt_node",
        "nav2_rate_controller_bt_node",
        "nav2_recovery_node_bt_node",
        "nav2_reinitialize_global_localization_service_bt_node",
        "nav2_remove_passed_goals_action_bt_node",
        "nav2_round_robin_node_bt_node",
        "nav2_single_trigger_bt_node",
        "nav2_smoother_selector_bt_node",
        "nav2_smooth_path_action_bt_node",
        "nav2_speed_controller_bt_node",
        "nav2_spin_action_bt_node",
        "nav2_spin_cancel_bt_node",
        "nav2_time_expired_condition_bt_node",
        "nav2_transform_available_condition_bt_node",
        "nav2_truncate_path_action_bt_node",
        "nav2_truncate_path_local_action_bt_node",
        "nav2_wait_action_bt_node",
        "nav2_wait_cancel_bt_node",
    };
    
    //executor.spin(); 
    std::thread executor_thread([&]() {
        executor.spin();  // Spin the executor in a separate thread
    });

    nav2_behavior_tree::BehaviorTreeEngine engine(plugins);
    
    auto blackboard = BT::Blackboard::create();
    blackboard->set("node", node);
    blackboard->set("bt_loop_duration", std::chrono::milliseconds(100));
    blackboard->set("server_timeout", std::chrono::milliseconds(10000));
    blackboard->set("wait_for_service_timeout", std::chrono::milliseconds(3000));
    
    load_yaml_file(waypoints_yaml_file, blackboard);

    auto tree = engine.createTreeFromFile(bt_xml_file, blackboard);
    
    BT::PublisherZMQ publisher(tree);
    
    auto onLoop = []() {
        //RCLCPP_INFO(rclcpp::get_logger("Main"), "Running behavior tree...");
    };
    auto cancelRequested = [&]() -> bool {
        return stop_requested_; // Stop BT if service was called
    };
    
    nav2_behavior_tree::BtStatus status = engine.run(&tree, onLoop, cancelRequested, std::chrono::milliseconds(10));

    if (status == nav2_behavior_tree::BtStatus::SUCCEEDED) {
        RCLCPP_INFO(rclcpp::get_logger("Main"), "Behavior Tree succeeded.");
    } else {
        RCLCPP_ERROR(rclcpp::get_logger("Main"), "Behavior Tree failed or was canceled.");
    }

    rclcpp::shutdown();
    executor_thread.join();
    return 0;
}

/* if you want to cancel bt
ros2 service call /edu_robot/bt_stop std_srvs/srv/SetBool "{data: true}"
*/