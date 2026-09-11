#include "rclcpp/rclcpp.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "rom_interfaces/srv/which_maps.hpp"
#include <filesystem>

#include <memory>
#include <std_msgs/msg/bool.hpp>  
#include <std_msgs/msg/string.hpp>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "rom_interfaces/srv/construct_yaml.hpp"
#include <sstream>
#include <iostream>

#define ROM_DEBUG 1

#ifndef ROM_DYNAMICS_UNUSED
#define ROM_DYNAMICS_UNUSED(x) (void)(x)
#endif
// switch mode parameters
pid_t launch_pid = -1;

// Package and launch file names
const std::string wp_package = "rom_waypoints_provider";                 
const std::string launch_all_goals = "send_waypoints_all_goals.launch.py";
const std::string launch_cus_goals = "send_waypoints_custom_goals.launch.py";
const std::string launch_all_goals_loop = "send_waypoints_all_goals_loop.launch.py";
const std::string launch_cus_goals_loop = "send_waypoints_custom_goals_loop.launch.py";

std::vector<std::string> wp_names_;
std::string data;

bool loop_state= false;
std::string nav_command="all_goals";

bool first_time = true;

// for publisher
rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr publisher;
int counter = 0;

//bool all_goals, all_goals_loop, custom_goals, custom_goals_loop = false;
void all_goals();
void all_goals_loop();
void custom_goals();
void custom_goals_loop();

std::string joinVector(const std::vector<std::string>& waypoints, const std::string& delimiter = " ") {
    std::ostringstream oss;
    for (size_t i = 0; i < waypoints.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << waypoints[i];
    }
    return oss.str();
}

// switch_mode functions
void startLaunch(const std::string &package, const std::string &launch_file, const std::string data_wp = "") 
{
      #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("wp_server"), "Starting launch file: %s/%s", package.c_str(), launch_file.c_str());
      #endif
        // Fork a proce  to run the launch file
        launch_pid = fork();
        if (launch_pid == 0) {
            // In child process
            if(!data_wp.empty())
            {
                execlp("ros2", "ros2", "launch", package.c_str(), launch_file.c_str(), data_wp, (char *)NULL);
            }
            else
            {
                execlp("ros2", "ros2", "launch", package.c_str(), launch_file.c_str(), (char *)NULL);
            }
            perror("execlp failed");
            std::exit(EXIT_FAILURE);
        }

        if (launch_pid < 0) {
            perror("fork failed");
            throw std::runtime_error("Failed to start launch process");
        }
}

void shutdownLaunch() 
{
        if (launch_pid > 0) {
          #ifdef ROM_DEBUG
            RCLCPP_INFO(rclcpp::get_logger("wp_server"), "Shutting down current launch process (PID: %d)...", launch_pid);
          #endif
            kill(launch_pid, SIGINT);
            int status;
            waitpid(launch_pid, &status, 0);
            launch_pid = -1;
            sleep(3); // Ensure the process has fully terminated
        }
}

void waypoints_select(const std::shared_ptr<rom_interfaces::srv::ConstructYaml::Request> request,
          std::shared_ptr<rom_interfaces::srv::ConstructYaml::Response> response)
{
    ROM_DYNAMICS_UNUSED(response);
    
    if(request->pose_names.size()<1) 
    {
      #ifdef ROM_DEBUG
          RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "There is no Waypoints Goals");
      #endif
      return; 
    }
  
    loop_state = request->loop;
    nav_command = request->command;

    if(nav_command == "all_goals" && loop_state == true)
    {
        
        all_goals_loop();
    }
    else if(nav_command == "all_goals" && loop_state == false)
    {
        
        all_goals();
    }
    else if(nav_command == "custom_goals" && loop_state == true)
    {
        // copy data from qt
        wp_names_.clear();
        data.clear();
            for (size_t i = 0; i < request->pose_names.size(); ++i) 
            {
                wp_names_.emplace_back(request->pose_names[i]);
                RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "request->pose_names[i] : %s",wp_names_[i].c_str());
            }
        data = joinVector(wp_names_);
        
        
        custom_goals_loop();
    }
    else if(nav_command == "custom_goals" && loop_state == false)
    {
        // copy data from qt
        wp_names_.clear();
        data.clear();
            for (size_t i = 0; i < request->pose_names.size(); ++i) 
            {
                wp_names_.emplace_back(request->pose_names[i]);
                RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "request->pose_names[i] : %s",wp_names_[i].c_str());
            }
        data = joinVector(wp_names_);
        
        
        custom_goals();
    }
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("waypoint_server");

    rclcpp::Service<rom_interfaces::srv::ConstructYaml>::SharedPtr service = node->create_service<rom_interfaces::srv::ConstructYaml>("waypoints_selected", &waypoints_select);
    publisher = node->create_publisher<std_msgs::msg::Bool>("navigation_stop", 10);

    #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "Ready to selected waypoints."); 
    #endif

    rclcpp::spin(node);
    rclcpp::shutdown();
}



// shutdown launch ခေါ်ဖို့လိုမလို စဥ်းစားပါ။ first time trigger
void all_goals()
{
    RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "all_goals()");
    // firstime param
    if(first_time)
    {
        startLaunch(wp_package,launch_all_goals);
        #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "New Launch all goals()");
        #endif
        first_time = false;
        return;
    }
    else
    {
        // check wp_loop_done is true if true, get param meter 
        // while(!wp_loop_done)
        {

        }
        shutdownLaunch();
        startLaunch(wp_package,launch_all_goals);
        #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "Restart Launch all goals()");
        #endif
    }
}

void all_goals_loop()
{
    RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "all_goals_loop()");
    if(first_time)
    {
        startLaunch(wp_package,launch_all_goals_loop);
        #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "New Launch all goals loops()");
        #endif
        first_time = false;
        return;
    }
    else
    {
        shutdownLaunch();
        startLaunch(wp_package,launch_all_goals_loop);
        #ifdef ROM_DEBUG
        RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "Restart Launch all goals loops()");
        #endif
    }
}

void custom_goals()
{
    RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "custom_goals()");
    if(first_time)
    {
        // //startLaunch(wp_package,launch_cus_goals, wp);
        // #ifdef ROM_DEBUG
        // RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "New Launch all custom_goals()");
        // #endif
        // first_time = false;
        // return;
    }
    else
    {

    }
    // shutdownLaunch();
    // startLaunch(wp_package,launch_all_goals, data);
}

void custom_goals_loop()
{
    RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "custom_goals_loop()");
    if(first_time)
    {
        // startLaunch(wp_package,launch_cus_goals_loop);
        // #ifdef ROM_DEBUG
        // RCLCPP_INFO(rclcpp::get_logger("send_waypoints_server"), "New Launch custom_goals_loop()");
        // #endif
        // first_time = false;
        // return;
    }
    else
    {

    }
    // shutdownLaunch();
    // startLaunch(wp_package,launch_all_goals, data);
}