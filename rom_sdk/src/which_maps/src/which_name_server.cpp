
#define IS_PRODUCTION 1

#include "rclcpp/rclcpp.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "rom_interfaces/srv/which_maps.hpp"
#include <filesystem>

#include <memory>

#include <std_msgs/msg/string.hpp>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "rom_define.h"

//#define ROM_DEBUG 1

float ROBOT_DIAMETER = 0.63; 

std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String>> global_publisher;

// Package and launch file names
const std::string rom_robot_namespace = std::getenv("ROM_ROBOT_NAMESPACE");
const std::string robot_name = std::getenv("ROM_ROBOT_MODEL");


bool debug_mode_ = false; // Global variable to control debug logging

void which_map_answer(const std::shared_ptr<rom_interfaces::srv::WhichMaps::Request> request,
          std::shared_ptr<rom_interfaces::srv::WhichMaps::Response>      response)
{
  /// HandShake
  if (request->request_string == "handshake")
  {
    std::string login_access_token = request->login_access_token;
    if( login_access_token == "*#5447972162718281828459#" ) // should be changed later
    {
      response->status = 14; // handshake ok
      if(rom_robot_namespace.empty()) 
      {
        response->robot_namespace = "simulation";

        if(debug_mode_)
        {
          RCLCPP_WARN(rclcpp::get_logger("which_name_server"), "ROM_ROBOT_NAMESPACE environment variable is not set. Using 'simulation' as robot namespace.");
        }
      }
      else 
      {
        response->robot_namespace = rom_robot_namespace;
      }
      response->robot_diameter = ROBOT_DIAMETER;

      if(debug_mode_)
      {
        RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "get handshake : Response Status OK");
        RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "get handshake : Response Status OK");
      }
    }
    else 
    {
      response->status = 15; // handshake not ok
      response->robot_namespace = "unknown";

      if(debug_mode_)
      {
        RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "get handshake : Response Status not OK");
        RCLCPP_WARN(rclcpp::get_logger("which_name_server"), "Unknown token '%s'.", request->map_name_to_save.c_str());
      }
    }
  }
  
  
  /// ဘာမှမလုပ်
  else 
  {
    response->total_maps = 0;
    response->status = -1; // not ok

    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "Sending : Response Status not OK");
      RCLCPP_WARN(rclcpp::get_logger("which_name_server"), "Unknown mode '%s'.", request->request_string.c_str());
    }
  }
}


int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  if (robot_name.empty()) 
  {
    throw std::runtime_error("robot_name is empty. Please set ROM_ROBOT_MODEL environment variable.");  
  }

  // Check debug mode environment variable
  const char* debug_env = std::getenv("ROM_DYNAMICS_DEBUG");
  debug_mode_ = (debug_env != nullptr && std::string(debug_env) == "true");
  RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "debug_mode_: %s", debug_mode_ ? "true" : "false");

  std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("which_name_server");

  rclcpp::Service<rom_interfaces::srv::WhichMaps>::SharedPtr service = node->create_service<rom_interfaces::srv::WhichMaps>("/which_name", &which_map_answer);

  if(debug_mode_)
  {
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Ready to answer names.");
    RCLCPP_INFO(rclcpp::get_logger("which_name_server"), "Fist Time trigger to Nav mode");
  }

  rclcpp::spin(node);
  rclcpp::shutdown();
}