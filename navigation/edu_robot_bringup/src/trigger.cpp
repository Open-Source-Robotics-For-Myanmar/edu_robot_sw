#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

int main(int argc, char * argv[])
{
 
  rclcpp::init(argc, argv);

  
  auto node = rclcpp::Node::make_shared("trigger_node");

  
  auto publisher = node->create_publisher<std_msgs::msg::String>("/output_topic", 10);
  while (rclcpp::ok() && publisher->get_subscription_count() == 0) {
    
    std::this_thread::sleep_for(500ms);
  }

  if (publisher->get_subscription_count() > 0) {
   
    auto message = std_msgs::msg::String();
    message.data = "navi";
    RCLCPP_INFO(rclcpp::get_logger("trigger_node"), "Publishing: '%s'", message.data.c_str());
    publisher->publish(message);
  } else {
    
    RCLCPP_INFO(rclcpp::get_logger("trigger_node"), "No subscribers, skipping publish.");
  }
  
  rclcpp::spin(node);

 
  rclcpp::shutdown();
  return 0;
}
