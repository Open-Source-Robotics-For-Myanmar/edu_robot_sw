#define IS_PRODUCTION 1

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include <geometry_msgs/msg/pose2_d.hpp>

#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"

#include <cmath>

// q: geometry_msgs::msg::Quaternion (x,y,z,w)
// return: yaw in degrees (−180º..+180º)
static inline double yawDegreeFromQuaternion(const geometry_msgs::msg::Quaternion& q)
{
  const double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
  const double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  const double yaw_rad   = std::atan2(siny_cosp, cosy_cosp);
  return yaw_rad * 180.0 / M_PI;
}

using namespace std::chrono_literals;

const std::string rom_robot_namespace = std::getenv("ROM_ROBOT_NAMESPACE");

class FrameListener : public rclcpp::Node
{
public:
  FrameListener(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("map_base_footprint_pub", rom_robot_namespace, options)
  {
    // Declare and acquire `target_frame` parameter
    //target_frame_ = "map";
    target_frame_ = "base_footprint";

    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Create turtle2 velocity publisher
    publisher_ =this->create_publisher<geometry_msgs::msg::Pose2D>("map_bfp_publisher", 10);

    // Call on_timer function every second
    //timer_ = this->create_wall_timer( 100ms, std::bind(&FrameListener::on_timer, this));
    // Production နှင့် Simulation ပေါ်မူတည်ပြီး Timer ခွဲခြားခြင်း
    #ifdef IS_PRODUCTION
        // စက်ရုပ်အစစ်အတွက်: Wall Time (တကယ့်အချိန်) အတိုင်း Run မည်
        timer_ = this->create_wall_timer(100ms, std::bind(&FrameListener::on_timer, this));
    #else
        // Simulation အတွက်: Gazebo Clock အတိုင်း လိုက်ပါ Run မည့် rclcpp::create_timer ကိုသုံးခြင်း
        timer_ = rclcpp::create_timer(
            this, // Node context ရယူရန် (Class သည် std::enable_shared_from_this ကို inherit လုပ်ထားရပါမည်)
            this->get_clock(),
            100ms,
            std::bind(&FrameListener::on_timer, this)
        );
    #endif
  }

private:
  void on_timer()
  {
    std::string fromFrameRel = target_frame_.c_str();
    //std::string toFrameRel = "base_footprint";
    std::string toFrameRel = "map";

    geometry_msgs::msg::Pose2D frame;
    geometry_msgs::msg::TransformStamped t;
    try 
    {
          t = tf_buffer_->lookupTransform(
            toFrameRel, fromFrameRel,
            tf2::TimePointZero);
            
            frame.x = t.transform.translation.x;
            frame.y = t.transform.translation.y;

            frame.theta = yawDegreeFromQuaternion(t.transform.rotation);
      publisher_->publish(frame);
    } 
    catch (const tf2::TransformException & ex) {
          RCLCPP_INFO(
            this->get_logger(), "Could not transform %s to %s: %s",
            toFrameRel.c_str(), fromFrameRel.c_str(), ex.what());
          return;
        }

    
  }

  
  rclcpp::TimerBase::SharedPtr timer_{nullptr};

  rclcpp::Publisher<geometry_msgs::msg::Pose2D>::SharedPtr publisher_{nullptr};
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::string target_frame_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // Constructor ထဲသို့ rclcpp::NodeOptions() ကို ပို့ပေးလိုက်ခြင်း
  rclcpp::spin(std::make_shared<FrameListener>(rclcpp::NodeOptions()));
  rclcpp::shutdown();
  return 0;
}
