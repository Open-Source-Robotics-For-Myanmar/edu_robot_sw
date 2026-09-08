#include "../include/rom_robot_autonomy/navigation_behaviors.h"
#include "yaml-cpp/yaml.h"
#include <string>

//------------------- SendNavGoal -------------------
namespace nav2_behavior_tree
{

SendNavGoal::SendNavGoal(const std::string &name, const BT::NodeConfiguration &config, rclcpp::Node::SharedPtr node_ptr)
    : BT::StatefulActionNode(name, config), node_ptr_{node_ptr} 
{
    client_ = rclcpp_action::create_client<NavigateToPose>(node_ptr_, "navigate_to_pose");
}
// node_(rclcpp::Node::make_shared("bt_goto_pose"))


BT::PortsList SendNavGoal::providedPorts()
{
    return {
        BT::InputPort<double>("x"),
        BT::InputPort<double>("y"),
        BT::InputPort<double>("theta")};
}

BT::NodeStatus SendNavGoal::onStart()
{
    double x, y, theta;
    if (!getInput("x", x) || !getInput("y", y) || !getInput("theta", theta))
    {
        RCLCPP_ERROR(node_ptr_->get_logger(), "Missing x, y, or theta input!");
        return BT::NodeStatus::FAILURE;
    }

    if (!client_->wait_for_action_server(std::chrono::seconds(5)))
    {
        RCLCPP_ERROR(node_ptr_->get_logger(), "Action server not available!");
        return BT::NodeStatus::FAILURE;
    }

    // Create the goal message
    auto goal_msg = NavigateToPose::Goal();
    goal_msg.pose.header.frame_id = "map";  // Adjust based on your TF setup
    goal_msg.pose.header.stamp = node_ptr_->now();
    goal_msg.pose.pose.position.x = x;
    goal_msg.pose.pose.position.y = y;

    // degree to radian
    double theta_radian = theta * M_PI / 180.0;
    // Convert theta (yaw) to quaternion
    tf2::Quaternion q;
    q.setRPY(0, 0, theta_radian);
    goal_msg.pose.pose.orientation = tf2::toMsg(q);

    // Send goal
    auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
    send_goal_options.result_callback =
        [this](const GoalHandle::WrappedResult &result)
        {
            if (result.code == rclcpp_action::ResultCode::SUCCEEDED)
            {
                result_received_ = true;
                success_ = true;
            }
            else
            {
                result_received_ = true;
                success_ = false;
            }
        };

    result_received_ = false;
    success_ = false;
    future_goal_handle_ = client_->async_send_goal(goal_msg, send_goal_options);

    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus SendNavGoal::onRunning()
{
    if (result_received_)
    {
        return success_ ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::RUNNING;
}

void SendNavGoal::onHalted()
{
    if (future_goal_handle_.valid())
    {
        auto goal_handle = future_goal_handle_.get();
        if (goal_handle)
        {
            client_->async_cancel_goal(goal_handle);
        }
    }
}

} // namespace nav2_behavior_tree

//------------------- GoToPose -------------------
GoToPose::GoToPose(const std::string &name,
                   const BT::NodeConfiguration &config,
                   rclcpp::Node::SharedPtr node_ptr)
    : BT::StatefulActionNode(name, config), node_ptr_(node_ptr)
{
  action_client_ptr_ = rclcpp_action::create_client<NavigateToPose>(node_ptr_, "/navigate_to_pose");
  done_flag_ = false;
}

BT::PortsList GoToPose::providedPorts()
{
  return {BT::InputPort<std::string>("loc")};
}

BT::NodeStatus GoToPose::onStart()
{
  // Get location key from port and read YAML file
  BT::Optional<std::string> loc = getInput<std::string>("loc");
  const std::string location_file = node_ptr_->get_parameter("location_file").as_string();

  YAML::Node locations = YAML::LoadFile(location_file);

  std::vector<float> pose_loc = locations[loc.value()].as<std::vector<float>>();

  // setup action client
  auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
  send_goal_options.result_callback = std::bind(&GoToPose::nav_to_pose_callback, this, std::placeholders::_1);

  // make pose
  auto goal_msg = NavigateToPose::Goal();
  goal_msg.pose.header.frame_id = "map";
  goal_msg.pose.pose.position.x = pose_loc[0];
  goal_msg.pose.pose.position.y = pose_loc[1];

  tf2::Quaternion q;
  q.setRPY(0, 0, pose_loc[2]);
  q.normalize(); // todo: why?
  goal_msg.pose.pose.orientation = tf2::toMsg(q);

  // send pose
  done_flag_ = false;
  action_client_ptr_->async_send_goal(goal_msg, send_goal_options);
  RCLCPP_INFO(node_ptr_->get_logger(), "Sent Goal to Nav2\n");
  return BT::NodeStatus::RUNNING;
}

BT::NodeStatus GoToPose::onRunning()
{
  if (done_flag_)
  {
    RCLCPP_INFO(node_ptr_->get_logger(), "[%s] Goal reached\n", this->name().c_str());
    return BT::NodeStatus::SUCCESS;
  }
  else
  {
    return BT::NodeStatus::RUNNING;
  }
}

void GoToPose::nav_to_pose_callback(const GoalHandleNav::WrappedResult &result)
{
  // If there is a result, we consider navigation completed.
  // bt_navigator only sends an empty message without status. Idk why though.

  if (result.result)
  {
    done_flag_ = true;
  }
}
