#define IS_PRODUCTION 1

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rom_define.h"
#include "rom_interfaces/srv/which_vel.hpp" // Replace with your package's service file if custom
#include "std_msgs/msg/string.hpp"

const char *ns_env = std::getenv("ROM_ROBOT_NAMESPACE");
const std::string rom_robot_namespace = (ns_env != nullptr) ? ns_env : "";
const double linear_vel = 0.40; // ပြင်ရန်
const double angular_vel = 0.20;

// #define ROM_DEBUG 1

class CmdVelServiceNode : public rclcpp::Node {
public:
  // CLI သို့မဟုတ် Launch ဖိုင်ကလာတဲ့ Node Parameters (use_sim_time အပါအဝင်) ကို လက်ခံနိုင်ရန်
  // NodeOptions ကို Constructor ထဲ ထည့်သွင်းထားပါတယ်
  CmdVelServiceNode(const rclcpp::NodeOptions &options = rclcpp::NodeOptions())
      : Node("which_vel_server", rom_robot_namespace, options) {
    publisher_ = this->create_publisher<geometry_msgs::msg::Twist>(
        "cmd_vel_stt_to_twist", 10);
    service_ = this->create_service<rom_interfaces::srv::WhichVel>(
        "which_vel", std::bind(&CmdVelServiceNode::handle_command, this,
                               std::placeholders::_1, std::placeholders::_2));

// Macro စစ်ပြီး Timer အမျိုးအစားကို ခွဲခြားသတ်မှတ်ခြင်း
#ifdef IS_PRODUCTION
    // Production အတွက်: Network/CPU ဘယ်လောက်တက်တက် စက္ကန့်အတိအကျသွားမယ့် Wall Timer ကိုသုံးမယ်
    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(50),
        std::bind(&CmdVelServiceNode::publish_cmd_vel, this));
    RCLCPP_INFO(
        this->get_logger(),
        "CmdVelServiceNode started in PRODUCTION mode (Using Wall Timer).");
#else
    // Simulation အတွက်: Gazebo Clock (use_sim_time) ကို လိုက်နာမယ့် rclcpp::create_timer
    // ကိုသုံးမယ်
    timer_ = rclcpp::create_timer(
        this, // shared pointer context ကို ရယူရန် (သို့မဟုတ် template သတ်မှတ်ချက်အရ "this" ဟု
              // ရေးနိုင်သည်)
        this->get_clock(), std::chrono::milliseconds(50),
        std::bind(&CmdVelServiceNode::publish_cmd_vel, this));
    RCLCPP_INFO(
        this->get_logger(),
        "CmdVelServiceNode started in SIMULATION mode (Using Sim Timer).");
#endif

    // ROM_DYNAMICS_DEBUG env var ကို စစ်ဆေးပြီး debug_mode_ သတ်မှတ်ခြင်း
    const char *debug_env = std::getenv("ROM_DYNAMICS_DEBUG");
    debug_mode_ = (debug_env != nullptr && std::string(debug_env) == "true");
    RCLCPP_INFO(this->get_logger(), "debug_mode_: %s",
                debug_mode_ ? "true" : "false");

#ifdef ROM_DEBUG
    RCLCPP_INFO(this->get_logger(), "CmdVelServiceNode started.");
#endif
  }

private:
  void handle_command(
      const std::shared_ptr<rom_interfaces::srv::WhichVel::Request> request,
      std::shared_ptr<rom_interfaces::srv::WhichVel::Response> response) {
    auto command = request->command;
    auto linear_x = request->linear_x;
    auto angular_z = request->angular_z;
    geometry_msgs::msg::Twist twist;

    // Parse the command and set twist values accordingly
    if (command == "forward") {
      twist.linear.x = linear_x;
      twist.angular.z = 0.0;
      should_publish_ = true;
    } else if (command == "stop") {
      twist.linear.x = 0.0;
      twist.angular.z = 0.0;
      should_publish_ = true;
      should_stop_ = true;
    } else if (command == "left") {
      twist.linear.x = 0.0;
      twist.angular.z = angular_z;
      should_publish_ = true;
    } else if (command == "right") {
      twist.linear.x = 0.0;
      twist.angular.z = angular_z;
      should_publish_ = true;
    } else {
#ifdef ROM_DEBUG
      RCLCPP_WARN(this->get_logger(), "Unknown command: '%s'", command.c_str());
#endif
      response->status = -1;
      twist = geometry_msgs::msg::Twist();
      should_publish_ = false;
      return;
    }
    current_twist_ = twist;
    response->status = 1;
  }

  void publish_cmd_vel() {
    if (should_publish_) {
      publisher_->publish(current_twist_);
      if (debug_mode_) {
        RCLCPP_INFO_ONCE(this->get_logger(), "Publishing cmd_vel...");
      }
      if (should_publish_ && should_stop_) {
        should_publish_ = false;
        should_stop_ = false;
      }
    }
  }

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
  rclcpp::Service<rom_interfaces::srv::WhichVel>::SharedPtr service_;
  rclcpp::TimerBase::SharedPtr timer_;

  geometry_msgs::msg::Twist current_twist_; // Stores the current Twist message
  bool should_publish_ = false;
  bool should_stop_ = false;
  bool debug_mode_ = false;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);

  // Class အသစ်ရဲ့ NodeOptions ကို pointer ဆောက်တဲ့နေရာမှာ လှမ်းထည့်ပေးလိုက်ပါတယ်
  rclcpp::spin(std::make_shared<CmdVelServiceNode>(rclcpp::NodeOptions()));

  rclcpp::shutdown();
  return 0;
}