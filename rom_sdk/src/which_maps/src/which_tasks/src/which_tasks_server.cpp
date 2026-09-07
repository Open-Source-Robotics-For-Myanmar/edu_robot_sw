#define IS_PRODUCTION 1

#include "rclcpp/rclcpp.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "rom_interfaces/srv/which_tasks.hpp"
#include <filesystem>
#include <fstream>

#include <memory>

#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <atomic>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>
//#include "rom_define.h"

//#define ROM_DEBUG 1

std::string package_name = "which_tasks";

const char *ns_env = std::getenv("ROM_ROBOT_NAMESPACE");
const std::string rom_robot_namespace = (ns_env != nullptr) ? ns_env : "";
const char *model_env = std::getenv("ROM_ROBOT_MODEL");
const std::string robot_name = (model_env != nullptr) ? model_env : "";

std::string package_directory = "/home/mr_robot/data/maps/";

std::atomic<bool> cmd_publish(false);
std::atomic<double> linear_speed(0.0);
std::atomic<double> angular_speed(0.0);
std::atomic<int> zero_publish_count(0);
std::string command_topic_name =  "cmd_vel_voice_command";

double LINEAR_SPEED = 0.30;
double ANGULAR_SPEED = 0.8;


bool debug_mode_ = false; // Global variable to control debug logging


void which_task_answer(const std::shared_ptr<rom_interfaces::srv::WhichTasks::Request> request,
          std::shared_ptr<rom_interfaces::srv::WhichTasks::Response>      response)
{
  /// ၈။ voice_command 
  if(request->request_string == "voice_command")
  {
    response->status = -5; // not ok
    std::string stt_command = request->request_settings;
    
    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Voice command received: %s", stt_command.c_str());
    }

    if( stt_command.find("forward") != std::string::npos || stt_command.find("ahead") != std::string::npos || stt_command.find("go") != std::string::npos )
    {
      response->status = 5; // ok
      linear_speed.store(LINEAR_SPEED);
      angular_speed.store(0.0);
      cmd_publish = true; 
    }
    else if( stt_command.find("backward") != std::string::npos || stt_command.find("back") != std::string::npos || stt_command.find("reverse") != std::string::npos )
    {
      response->status = 5; // ok
      linear_speed.store(-LINEAR_SPEED);
      angular_speed.store(0.0);
      cmd_publish = true;
    }
    else if( stt_command.find("left") != std::string::npos || stt_command.find("turn left") != std::string::npos )
    {
      response->status = 5; // ok
      linear_speed.store(0.0);
      angular_speed.store(ANGULAR_SPEED);
      cmd_publish = true;
    }
    else if( stt_command.find("right") != std::string::npos || stt_command.find("turn right") != std::string::npos )
    {
      response->status = 5; // ok
      linear_speed.store(0.0);
      angular_speed.store(-ANGULAR_SPEED);
      cmd_publish = true;
    }
    else if( stt_command.find("stop") != std::string::npos || stt_command.find("halt") != std::string::npos || stt_command.find("emergency") != std::string::npos )
    {
      response->status = 5; // ok
      linear_speed.store(0.0);
      angular_speed.store(0.0);
      cmd_publish = false;
    }
    else
    {
      response->status = -5; // not ok
      cmd_publish = false;
    }
    
    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Voice command received: %s", stt_command.c_str());
    }
  }
  /// ၁။ 
  else if(request->request_string == "call_from_table")
  {
    
  }
  /// ၂။ 
  else if(request->request_string == "video")
  {
    
  }
  /// ၃။ 
  else if(request->request_string == "sound")
  {
    
  }
  /// ၄။ 
  else if(request->request_string == "setting")
  {
    // read setting file and send back
    std::string setting_path = "/home/mr_robot/data/setting/setting.yaml";

    std::ifstream setting_file(setting_path);
    if (setting_file.is_open())
    {
      std::string line;
      std::string settings_content;
      while (std::getline(setting_file, line))
      {
        settings_content += line + "\n";
      }
      setting_file.close();
      response->response_settings = settings_content;
      response->status = 1; // ok
    }
    else
    {
      response->status = -1; // not ok
    }
  }
  /// ၅။ 
  else if(request->request_string == "save_setting")
  {
    // save setting file from request
    std::string setting_path = "/home/mr_robot/data/setting/setting.yaml";

    std::ofstream setting_file(setting_path);
    if (setting_file.is_open())
    {
      setting_file << request->request_settings;
      setting_file.close();
      response->status = 2; // ok
    }
    else
    {
      response->status = -2; // not ok
    }
  }
  /// ၆။ 
  else if(request->request_string == "reboot")
  {
    response->status = 3; // ok
    response->response_settings = "Rebooting system...";
    
    // Execute reboot command in a separate thread to allow response to be sent first
    std::thread([]() {
      std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1 second to send response
      system("sudo reboot");
    }).detach();
    
    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Reboot command initiated");
    }
  }
  /// ၇။ 
  else if(request->request_string == "poweroff")
  {
    response->status = 4; // ok
    response->response_settings = "Powering off system...";
    
    // Execute poweroff command in a separate thread to allow response to be sent first
    std::thread([]() {
      std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1 second to send response
      system("sudo poweroff");
    }).detach();
    
    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Poweroff command initiated");
    }
  }

  /// ၈။ save ai api key
  else if(request->request_string == "save_ai_api_key")
  {
    std::string provider = request->request_settings;
    std::string api_key  = request->str_reserve2;

    if (provider == "Gemini")
    {
      // Check if GEMINI_API_KEY exists, if yes replace it, else append it
      std::string cmd = "grep -q '^export GEMINI_API_KEY=' /home/mr_robot/data/systemd/.rom_environment.sh "
                        "&& sed -i 's/^export GEMINI_API_KEY=.*/export GEMINI_API_KEY=\\\"" + api_key + "\\\"/' /home/mr_robot/data/systemd/.rom_environment.sh "
                        "|| echo 'export GEMINI_API_KEY=\\\"" + api_key + "\\\"' >> /home/mr_robot/data/systemd/.rom_environment.sh";
      
      int ret = system(cmd.c_str());
      
      // Ensure success status from WEXITSTATUS
      if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        response->status = 6; // ok
      } else {
        response->status = -6; // not ok
      }

      if(debug_mode_)
      {
        RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Saved Gemini API Key, status=%d", response->status);
      }
    }
    else if (provider == "OpenAI")
    {
      // TODO: Implement OpenAI save logic
      response->status = -6;
    }
    else if (provider == "RomServer")
    {
      // TODO: Implement RomServer save logic
      response->status = -6;
    }
    else if (provider == "LocalWhisper")
    {
      // TODO: Implement LocalWhisper save logic
      response->status = -6;
    }
    else
    {
      RCLCPP_ERROR(rclcpp::get_logger("which_tasks_server"), "Unknown API provider mapping: %s", provider.c_str());
      response->status = -6;
    }
  }

///  ဘာမှမလုပ်
  else 
  {
    // response->total_maps = 0;
    response->status = -1; // not ok

    if(debug_mode_)
    {
      RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "Sending : Response Status not OK");
      RCLCPP_WARN(rclcpp::get_logger("which_tasks_server"), "Unknown mode '%s'.", request->request_string.c_str());
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

  // ROM_DYNAMICS_DEBUG env var ကို စစ်ဆေးပြီး debug_mode_ သတ်မှတ်ခြင်း
  const char* debug_env = std::getenv("ROM_DYNAMICS_DEBUG");
  debug_mode_ = (debug_env != nullptr && std::string(debug_env) == "true");
  RCLCPP_INFO(rclcpp::get_logger("which_tasks_server"), "debug_mode_: %s", debug_mode_ ? "true" : "false");


  // === ပြင်ဆင်ချက် ၁။ CLI ကလာတဲ့ use_sim_time ကို ဖတ်နိုင်အောင် NodeOptions ထည့်သွင်းခြင်း ===
  auto node_options = rclcpp::NodeOptions();
  std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("which_tasks_server", rom_robot_namespace, node_options);

  rclcpp::Service<rom_interfaces::srv::WhichTasks>::SharedPtr service = node->create_service<rom_interfaces::srv::WhichTasks>("which_tasks", &which_task_answer);

  // Create cmd_vel publisher
  auto cmd_vel_pub = node->create_publisher<geometry_msgs::msg::Twist>(command_topic_name, 10);

  // Lambda ထဲမှာ သုံးဖို့အတွက် Logger ကို သီးသန့်ထုတ်ယူထားခြင်း (Crash မဖြစ်စေရန်)
  auto node_logger = node->get_logger();

  // === ပြင်ဆင်ချက် ၂။ Macro သုံးပြီး Production နှင့် Simulation အတွက် Timer ခွဲခြားခြင်း ===
  #ifdef IS_PRODUCTION
    // Production (ရိုဘော့အစစ်) အတွက်: Wall Timer ကိုသုံးမည်
    auto timer = node->create_wall_timer(
      std::chrono::milliseconds(50),
      [&, node_logger]() {
  #else
    // Simulation (Gazebo) အတွက်: Sim Timer ကိုသုံးမည်
    auto timer = rclcpp::create_timer(
      node,
      node->get_clock(),
      std::chrono::milliseconds(50),
      [&, node_logger]() {
  #endif
        auto twist_msg = geometry_msgs::msg::Twist();
        
        if (cmd_publish.load()) 
        {
          zero_publish_count.store(0);
          
          twist_msg.linear.x = linear_speed.load();
          twist_msg.angular.z = angular_speed.load();
          cmd_vel_pub->publish(twist_msg);
          
          if(debug_mode_){
            RCLCPP_INFO(node_logger, "Publishing cmd_vel: linear=%.2f, angular=%.2f", 
                        twist_msg.linear.x, twist_msg.angular.z);
          }
        } else 
        {
          int count = zero_publish_count.load();
          if (count < 5) {
            twist_msg.linear.x = 0.0;
            twist_msg.angular.z = 0.0;
            cmd_vel_pub->publish(twist_msg);
            zero_publish_count.store(count + 1);
            
            if(debug_mode_){
              RCLCPP_INFO(node_logger, "Publishing zero cmd_vel (%d/5)", count + 1);
            }
          }
        }
      });

  if(debug_mode_){
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Ready to answer tasks.");
    RCLCPP_INFO(node->get_logger(), "Velocity publisher active on topic: %s", command_topic_name.c_str());
  }

  // Logging mode state
  #ifdef IS_PRODUCTION
    RCLCPP_INFO(node->get_logger(), "Which Tasks Server started in PRODUCTION mode.");
  #else
    RCLCPP_INFO(node->get_logger(), "Which Tasks Server started in SIMULATION mode.");
  #endif

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}