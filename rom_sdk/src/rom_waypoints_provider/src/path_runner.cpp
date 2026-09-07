#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_through_poses.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <yaml-cpp/yaml.h>

#define ROM_DEBUG 1

using NavigateThroughPoses = nav2_msgs::action::NavigateThroughPoses;
using GoalHandleNavigateThroughPoses = rclcpp_action::ClientGoalHandle<NavigateThroughPoses>;

class PathRunner : public rclcpp::Node
{
public:
  PathRunner()
  : Node("path_runner_node")
  {
    this->declare_parameter<std::string>("yaml_path", "/home/buc_robot/data/waypoints/path_mode.yaml");
    
    // Create action client
    client_ = rclcpp_action::create_client<NavigateThroughPoses>(
      this,
      "navigate_through_poses");

    #ifdef ROM_DEBUG
      RCLCPP_INFO(this->get_logger(), "Path Runner node initialized");
    #endif

    // Wait for action server
    if (!client_->wait_for_action_server(std::chrono::seconds(10))) {
      RCLCPP_ERROR(this->get_logger(), "Action server not available after waiting");
      rclcpp::shutdown();
      return;
    }

    // Create cancel service
    cancel_service_ = this->create_service<std_srvs::srv::SetBool>(
      "path_runner/cancel",
      std::bind(&PathRunner::cancel_callback, this, std::placeholders::_1, std::placeholders::_2));

    #ifdef ROM_DEBUG
      RCLCPP_INFO(this->get_logger(), "Cancel service created at: path_runner/cancel");
    #endif

    // Load waypoints and send goal
    if (load_waypoints_from_yaml()) {
      send_goal();
    } else {
      RCLCPP_ERROR(this->get_logger(), "Failed to load waypoints from YAML");
      rclcpp::shutdown();
    }
  }

private:
  rclcpp_action::Client<NavigateThroughPoses>::SharedPtr client_;
  std::vector<geometry_msgs::msg::PoseStamped> waypoints_;
  GoalHandleNavigateThroughPoses::SharedPtr goal_handle_;
  rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr cancel_service_;

  bool load_waypoints_from_yaml()
  {
    std::string yaml_path = this->get_parameter("yaml_path").as_string();

    #ifdef ROM_DEBUG
      RCLCPP_INFO(this->get_logger(), "Loading waypoints from: %s", yaml_path.c_str());
    #endif

    try {
      YAML::Node config = YAML::LoadFile(yaml_path);
      
      if (!config["waypoints"]) {
        RCLCPP_ERROR(this->get_logger(), "No 'waypoints' field found in YAML");
        return false;
      }

      for (const auto& wp : config["waypoints"]) {
        geometry_msgs::msg::PoseStamped pose_stamped;
        
        // Set frame_id
        if (wp["frame_id"]) {
          pose_stamped.header.frame_id = wp["frame_id"].as<std::string>();
        } else {
          pose_stamped.header.frame_id = "map";
        }
        
        pose_stamped.header.stamp = this->now();

        // Extract position
        if (wp["pose"]["position"]) {
          pose_stamped.pose.position.x = wp["pose"]["position"]["x"].as<double>();
          pose_stamped.pose.position.y = wp["pose"]["position"]["y"].as<double>();
          pose_stamped.pose.position.z = wp["pose"]["position"]["z"].as<double>();
        }

        // Extract orientation
        if (wp["pose"]["orientation"]) {
          pose_stamped.pose.orientation.x = wp["pose"]["orientation"]["x"].as<double>();
          pose_stamped.pose.orientation.y = wp["pose"]["orientation"]["y"].as<double>();
          pose_stamped.pose.orientation.z = wp["pose"]["orientation"]["z"].as<double>();
          pose_stamped.pose.orientation.w = wp["pose"]["orientation"]["w"].as<double>();
        }

        waypoints_.push_back(pose_stamped);

        #ifdef ROM_DEBUG
          std::string name = wp["name"] ? wp["name"].as<std::string>() : "unnamed";
          RCLCPP_INFO(this->get_logger(), "Loaded waypoint: %s [x: %.2f, y: %.2f]", 
                      name.c_str(), pose_stamped.pose.position.x, pose_stamped.pose.position.y);
        #endif
      }

      #ifdef ROM_DEBUG
        RCLCPP_INFO(this->get_logger(), "Successfully loaded %zu waypoints", waypoints_.size());
      #endif

      return !waypoints_.empty();

    } catch (const YAML::Exception& e) {
      RCLCPP_ERROR(this->get_logger(), "YAML parsing error: %s", e.what());
      return false;
    } catch (const std::exception& e) {
      RCLCPP_ERROR(this->get_logger(), "Error loading YAML: %s", e.what());
      return false;
    }
  }

  void send_goal()
  {
    auto goal_msg = NavigateThroughPoses::Goal();
    goal_msg.poses = waypoints_;

    #ifdef ROM_DEBUG
      RCLCPP_INFO(this->get_logger(), "Sending goal with %zu waypoints", waypoints_.size());
    #endif

    auto send_goal_options = rclcpp_action::Client<NavigateThroughPoses>::SendGoalOptions();
    
    send_goal_options.goal_response_callback =
      std::bind(&PathRunner::goal_response_callback, this, std::placeholders::_1);
    
    send_goal_options.feedback_callback =
      std::bind(&PathRunner::feedback_callback, this, std::placeholders::_1, std::placeholders::_2);
    
    send_goal_options.result_callback =
      std::bind(&PathRunner::result_callback, this, std::placeholders::_1);

    client_->async_send_goal(goal_msg, send_goal_options);
  }

  void goal_response_callback(const GoalHandleNavigateThroughPoses::SharedPtr & goal_handle)
  {
    if (!goal_handle) {
      RCLCPP_ERROR(this->get_logger(), "Goal was rejected by server");
    } else {
      goal_handle_ = goal_handle;
      RCLCPP_INFO(this->get_logger(), "Goal accepted by server, waiting for result");
    }
  }

  void feedback_callback(
    GoalHandleNavigateThroughPoses::SharedPtr,
    const std::shared_ptr<const NavigateThroughPoses::Feedback> feedback)
  {
    #ifdef ROM_DEBUG
      RCLCPP_INFO(this->get_logger(), 
                  "Navigation in progress - Distance remaining: %.2f m",
                  feedback->distance_remaining);
    #endif
  }

  void result_callback(const GoalHandleNavigateThroughPoses::WrappedResult & result)
  {
    switch (result.code) {
      case rclcpp_action::ResultCode::SUCCEEDED:
        RCLCPP_INFO(this->get_logger(), "Goal succeeded!");
        break;
      case rclcpp_action::ResultCode::ABORTED:
        RCLCPP_ERROR(this->get_logger(), "Goal was aborted");
        break;
      case rclcpp_action::ResultCode::CANCELED:
        RCLCPP_WARN(this->get_logger(), "Goal was canceled");
        break;
      default:
        RCLCPP_ERROR(this->get_logger(), "Unknown result code");
        break;
    }
    goal_handle_.reset();
    rclcpp::shutdown();
  }

  void cancel_callback(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response)
  {
    if (request->data) {
      if (goal_handle_) {
        RCLCPP_INFO(this->get_logger(), "Canceling navigation goal...");
        auto cancel_result_future = client_->async_cancel_goal(goal_handle_);
        
        // Wait for cancel to complete
        if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), cancel_result_future) ==
            rclcpp::FutureReturnCode::SUCCESS)
        {
          RCLCPP_INFO(this->get_logger(), "Goal successfully canceled");
          response->success = true;
          response->message = "Navigation canceled successfully";
        } else {
          RCLCPP_ERROR(this->get_logger(), "Failed to cancel goal");
          response->success = false;
          response->message = "Failed to cancel navigation";
        }
      } else {
        RCLCPP_WARN(this->get_logger(), "No active goal to cancel");
        response->success = false;
        response->message = "No active navigation goal";
      }
    } else {
      response->success = false;
      response->message = "Set data=true to cancel navigation";
    }
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<PathRunner>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}


/*
HOW TO STOP
# Navigation ကို ရပ်ချင်ရင်
ros2 service call /path_runner/cancel std_srvs/srv/SetBool "{data: true}"
*/