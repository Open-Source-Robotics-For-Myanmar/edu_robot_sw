#define IS_PRODUCTION 1

#include "rclcpp/rclcpp.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "rom_interfaces/srv/which_maps.hpp"
#include <filesystem>
#include <memory>
#include <std_msgs/msg/string.hpp>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "rom_define.h"

std::string package_name = "which_maps";

std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String>> global_publisher;
auto trigger_msg = std_msgs::msg::String();

// switch mode parameters
std::string current_mode = "navi";
pid_t launch_pid = -1;

// Package and launch file names
const std::string rom_robot_namespace = std::getenv("ROM_ROBOT_NAMESPACE") ? std::getenv("ROM_ROBOT_NAMESPACE") : "";
const std::string robot_name = std::getenv("ROM_ROBOT_MODEL") ? std::getenv("ROM_ROBOT_MODEL") : "";

const std::string cartographer_pkg = robot_name + "_carto";                 
const std::string carto_mapping_launch = "cartographer_mapping.launch.py";

const std::string carto_localization_launch = "cartographer_localization.launch.py";

const std::string remapping_launch = "remapping.launch.py";


std::string package_directory = "/home/mr_robot/data/maps/";
bool debug_mode_ = false; // Global variable to control debug logging

// switch_mode functions
void startLaunch(const std::string &package, const std::string &launch_file) 
{
    if(debug_mode_)
    {
        RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Starting launch file: %s/%s", package.c_str(), launch_file.c_str());
    }
    
    launch_pid = fork();
    if (launch_pid == 0) {
        // In child process
        #ifdef IS_PRODUCTION
            execlp("ros2", "ros2", "launch", package.c_str(), launch_file.c_str(), (char *)NULL);
        #else
            execlp("ros2", "ros2", "launch", package.c_str(), launch_file.c_str(), "use_sim_time:=true", (char *)NULL);
        #endif

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
        if(debug_mode_)
        {
            RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Shutting down current launch process (PID: %d)...", launch_pid);
        }
        kill(launch_pid, SIGINT);
        int status;
        waitpid(launch_pid, &status, 0);
        launch_pid = -1;
        sleep(3); // Ensure the process has fully terminated
    }
}




// TODO 1. GO TO HOME 'S MAP DIRECTORY OR NOT
// INPUT ( which_map_do_you_have | save_map | select_map | mapping | navi | remapping )

// # -1 [service not ok], 1 [service ok], 
// # 2 [maps exist], 3 [maps do not exist], 
// # 4 [save map ok], 5 [save map not ok]
// # 6 [select map ok], 7 [select map not ok], 

// # 8 [mapping mode ok], 9 [mapping mode not ok],
// # 10 [navi mode ok], 11 [navi mode not ok]
// # 12 [remapping mode ok], 13 [remapping mode not ok]


void which_map_answer(const std::shared_ptr<rom_interfaces::srv::WhichMaps::Request> request,
                      std::shared_ptr<rom_interfaces::srv::WhichMaps::Response>      response)
{
    /// ၁။ ဘယ်မြေပုံတွေရှိလဲ?
    if(request->request_string == "which_maps_do_you_have"){
        // if(check existance of map directory)
    
        //RCLCPP_INFO(rclcpp::get_logger("which_maps"), "%s", package_directory.c_str());
        int yaml_file_count = 0;
        //std::string[] name_array;

        try 
        {
            for (const auto& entry : std::filesystem::directory_iterator(package_directory)) 
            {
                // active_map.yaml ကို စာရင်းထဲမထည့်ဘဲ ချန်လှပ်ထားရန်
                if (entry.is_regular_file() && entry.path().extension() == ".yaml" && entry.path().filename() != "active_map.yaml") //&& entry.path().extension() == ".pgm" && entry.path().extension() == ".pbstream") 
                {
                    response->map_names.push_back(entry.path().filename().string());
                    if(debug_mode_)
                    {
                        RCLCPP_INFO(rclcpp::get_logger("which_maps"), "%s", entry.path().filename().c_str());
                    }
                    ++yaml_file_count;
                }
            }
            std::cout << "Total .yaml files: " << yaml_file_count << std::endl;
            response->status = 2; // ok
            response->total_maps = yaml_file_count;
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status OK");
            }
        } 
        catch (const std::filesystem::filesystem_error& e) 
        {
            std::cerr << "Error accessing directory: " << e.what() << std::endl;
            response->total_maps = 0;
            response->status = 3; // not ok
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status not OK");
            }
        }
    }

    /// ၂။ မြေပုံ save ပါ။
    else if (request->request_string == "save_map"){
        std::string map_name = request->map_name_to_save;
        if(debug_mode_)
        {
            RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Saving map: %s", map_name.c_str());
        }
        // Save the map
        // command စစ်ဆေးရန်။

        // ၁။ pbstream မြေပုံထုတ်ယူခြင်း
        std::string cmd;
        #ifdef IS_PRODUCTION
            cmd = "cd " + package_directory + " && rm -f " + map_name + "* && ros2 service call /" + rom_robot_namespace + "/write_state cartographer_ros_msgs/srv/WriteState \"{filename: '" + package_directory + map_name + ".pbstream', include_unfinished_submaps: true}\"";
        #else
            cmd = "cd " + package_directory + " && rm -f " + map_name + "* && ros2 service call /" + rom_robot_namespace + "/write_state cartographer_ros_msgs/srv/WriteState \"{filename: '" + package_directory + map_name + ".pbstream', include_unfinished_submaps: true}\" --ros-args -p use_sim_time:=true";
        #endif

        int ret_code = std::system(cmd.c_str());
        if(ret_code == 0) 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_map_server"), "Map saver command 1 executed successfully.");
            }
            
            // ၂။ pbstream -> pgm, yaml ပြောင်းလဲခြင်း
            std::string cmd2 = "ros2 run cartographer_ros cartographer_pbstream_to_ros_map "
                               "-pbstream_filename " + package_directory + map_name + ".pbstream "
                               "-map_filename " + package_directory + map_name + ".pgm "
                               "-yaml_filename " + package_directory + map_name + ".yaml && "
                               "mv /home/mr_robot/rom_nav2_ws/map.pgm " + package_directory + map_name + ".pgm 2>/dev/null || true && "
                               "mv /home/mr_robot/rom_nav2_ws/map.yaml " + package_directory + map_name + ".yaml 2>/dev/null || true";
            
            int ret_code2 = std::system(cmd2.c_str());
            if (ret_code2 == 0)
            {
                if(debug_mode_)
                {
                    RCLCPP_INFO(rclcpp::get_logger("which_map_server"), "Map saver command 2 executed successfully.");
                }
                // ၃။ ထွက်လာတဲ့ yaml ဖိုင်ထဲက image name အား တိုက်ရိုက် ဆောက်ထားတဲ့ pgm သို့ လွှဲပြောင်းပြင်ဆင်ခြင်း 
                // (--copy သုံးထားခြင်းဖြင့် Device Busy သက်သာစေသည်)
                std::string cmd3 = "sed -i \"s|^image: .*\\.pgm|image: " + map_name + ".pgm|\" " + package_directory + map_name + ".yaml";
                //std::system(cmd3.c_str());

                int ret_code3 = std::system(cmd3.c_str());
                if(ret_code3==0) 
                { 
                    if(debug_mode_)
                    {
                    RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "sed command 3 Ok. all done."); response->status = 4;
                    }

                    // ၄။ အသစ်သိမ်းဆည်းလိုက်သော မြေပုံအား လက်ရှိသုံးမည့် Active Symlink အဖြစ် ချက်ချင်းချိတ်ဆက်ခြင်း
                    std::string ln_cmd = "ln -sf " + package_directory + map_name + ".yaml " + package_directory + "active_map.yaml && "
                                        "ln -sf " + package_directory + map_name + ".pgm " + package_directory + "active_map.pgm && "
                                        "ln -sf " + package_directory + map_name + ".pbstream " + package_directory + "active_map.pbstream";
                    std::system(ln_cmd.c_str());

                    if(debug_mode_) RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Map saved and symlinked successfully.");
                    response->status = 4; // save ok
                }
                else 
                { 
                    if(debug_mode_)
                    {
                    RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "sed command 3 Fail"); response->status = 5;
                    }
                    response->status = 5; // not ok
                } // cmd3 ret code check

                
            } 
            else 
            {
                if(debug_mode_)
                {
                    RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "sed command 2 Fail"); response->status = 5;
                }
                response->status = 5; // not ok
            } // cmd2 ret code check
        }
        else 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "sed command 1 Fail"); response->status = 5;
            }
            response->status = 5; // not ok
        } // cmd1 ret code check
    }

    /// ၃။ မြေပုံရွေးပါ။
    else if (request->request_string == "select_map")
    {   
        std::string map_name = request->map_name_to_select;
        if(map_name.empty())
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("Open Map"), "Sending : No Map Name ");
            }
            response->status = 7; // not ok
            return;
        }
        if(debug_mode_)
        {
            RCLCPP_INFO(rclcpp::get_logger("Open Map"), "Sending : Response Status OK");
        }


        ////////////////////////////////////////////////////////////////////////////////////////
        // ၁ ၊ name.pgm , name.yaml , name.pbstream ရှိမရှိစစ်ဆေးပါ။ မရှိရင် error (7)

        std::string pgm_file = package_directory + map_name + ".pgm";
        std::string yaml_file = package_directory + map_name + ".yaml";
        std::string pbstream_file = package_directory + map_name + ".pbstream";

        if(debug_mode_)
        {
            RCLCPP_INFO_STREAM(rclcpp::get_logger("Select Map"), pgm_file);
        }

        // မြေပုံဖိုင်များ အမှန်တကယ်ရှိမရှိ စစ်ဆေးခြင်း
        if (std::filesystem::exists(pgm_file) && std::filesystem::exists(yaml_file) && std::filesystem::exists(pbstream_file)) 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : All required files exist for map:");
            }
            // [အဓိကအချက်] - sed နေရာတွင် Linux Symlink (ဖြတ်လမ်းဖိုင်) ဖြင့် လမ်းကြောင်းပြောင်းလဲခြင်း
            // active_map အမည်ဖြင့် သက်ဆိုင်ရာ မြေပုံဆီ လှည့်ပေးလိုက်သည် (Container Mount Error ကင်းဝေးစေသည်)
            std::string symlink_cmd = "ln -sf " + yaml_file + " " + package_directory + "active_map.yaml && "
                                      "ln -sf " + pgm_file + " " + package_directory + "active_map.pgm && "
                                      "ln -sf " + pbstream_file + " " + package_directory + "active_map.pbstream";
            
            int link_ret = std::system(symlink_cmd.c_str());
            if(link_ret == 0)
            {
                if(debug_mode_)
                {
                    RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : Map files Linking is ok:");
                }

                // Nav2 Map Server သို့ မြေပုံအသစ်အား Runtime အရှင်လှမ်းဆွဲထည့်ခြင်း
                std::string path_to_active_yaml = package_directory + "active_map.yaml";
                std::string map_cmd;
                
                #ifdef IS_PRODUCTION
                    map_cmd = "ros2 service call /map_server/load_map nav2_msgs/srv/LoadMap \"{map_url: '" + path_to_active_yaml + "'}\" > /dev/null 2>&1";
                #else
                    map_cmd = "ros2 service call /map_server/load_map nav2_msgs/srv/LoadMap \"{map_url: '" + path_to_active_yaml + "'}\" --ros-args -p use_sim_time:=true > /dev/null 2>&1";
                #endif
                  
                int map_ret_code = std::system(map_cmd.c_str());
                if (map_ret_code == 0)
                {
                    if(debug_mode_)
                    {
                        RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : Map Loading is ok:");
                    }

                    // Costmaps များအား ရှင်းလင်းခြင်း
                    std::string clear_costmap_cmd = "ros2 service call /global_costmap/clear_around_global_costmap nav2_msgs/srv/ClearCostmapAroundRobot && ros2 service call /local_costmap/clear_around_local_costmap nav2_msgs/srv/ClearCostmapAroundRobot";
                    std::system(clear_costmap_cmd.c_str());

                    // Cartographer Localization ကို ပိတ်ပြီး အသစ်ပြန်ဖွင့်ခြင်း (Symlink အသစ်ကို ယူသုံးသွားမည်)
                    shutdownLaunch();
                    startLaunch(cartographer_pkg, carto_localization_launch);
                    
                    response->status = 6; // select map ok
                }
                else 
                {
                    if(debug_mode_)
                    {
                        RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : Map Loading is not ok:");
                    }
                    response->status = 7;
                } // Nav2 map load ret code check
            }
            else 
            {
                if(debug_mode_)
                {
                    RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : Map files Linking is not ok:");
                }
                response->status = 7;
            } // symlink ret code check
        } 
        else 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("Select Map"), " : Some required files are missing for map:");
            }
            response->status = 7; // ဖိုင်မစုံလင်ပါ
        } // map files existence check
    }

    /// ၄။ mapping mode change ပါ။
    else if (request->request_string == "mapping")
    {
        if (current_mode != "mapping") 
        {
            shutdownLaunch();
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status OK");
            }
            response->status = 8; // ok
            startLaunch(cartographer_pkg, carto_mapping_launch);
            current_mode = "mapping";
            trigger_msg.data = "mapping";
            global_publisher->publish(trigger_msg);
        }
        else 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Mode '%s' is already active.", current_mode.c_str());
            }
        }
    }

    /// ၅။ nav mode change ပါ။
    else if (request->request_string == "navi")
    {
        if (current_mode != "navi") 
        {
            shutdownLaunch();
            response->status = 10; // ok
            startLaunch(cartographer_pkg, carto_localization_launch);
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status OK");
            }
            current_mode = "navi";
            trigger_msg.data = "navi";
            global_publisher->publish(trigger_msg);
        }
        else
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Mode '%s' is already active.", current_mode.c_str());
            }
        }
    }
  
    /// ၆။ remapping mode change ပါ။
    else if (request->request_string == "remapping")
    {
        if (current_mode != "remapping") 
        {
            shutdownLaunch();
            startLaunch(cartographer_pkg, remapping_launch);
            response->status = 12; // ok
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status OK");
            }
            current_mode = "remapping";
            trigger_msg.data = "remapping";
            global_publisher->publish(trigger_msg);
        }
        else 
        {
            if(debug_mode_)
            {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Mode '%s' is already active.", request->request_string.c_str());
            }
        }   
    }

    /// ၇။ သတ်မှတ်ချက်မရှိသော mode ဖြစ်ပါက
    else 
    {
        response->total_maps = 0;
        response->status = -1;
        if(debug_mode_)
        {
            RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Sending : Response Status not OK");
            RCLCPP_WARN(rclcpp::get_logger("which_maps_server"), "Unknown mode '%s'.", request->request_string.c_str());
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
    RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "debug_mode_: %s", debug_mode_ ? "true" : "false");
  
    auto node_options = rclcpp::NodeOptions();
    std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("which_maps_server", rom_robot_namespace, node_options);

    rclcpp::Service<rom_interfaces::srv::WhichMaps>::SharedPtr service = node->create_service<rom_interfaces::srv::WhichMaps>("which_maps", &which_map_answer);

    // latch publisher
    global_publisher = node->create_publisher<std_msgs::msg::String>("which_nav", rclcpp::QoS(1).transient_local());

    if(debug_mode_)
    {
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Ready to answer maps.");
        RCLCPP_INFO(rclcpp::get_logger("which_maps_server"), "Fist Time trigger to Nav mode");
    }

    // ကနဦး စတင်ပတ်ချိန်တွင် လက်ရှိရှိနေပြီးသား active_map Symlink ဖြင့် Localization ကို တိုက်ရိုက်ပတ်မည်
    startLaunch(cartographer_pkg, carto_localization_launch);
  
    trigger_msg.data = current_mode;
    global_publisher->publish(trigger_msg);

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}