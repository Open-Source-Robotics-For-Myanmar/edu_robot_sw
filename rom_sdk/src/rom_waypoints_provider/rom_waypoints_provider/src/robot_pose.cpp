#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include <cstdlib>  // for getenv

class OdomToPosePublisher : public rclcpp::Node
{
public:
    OdomToPosePublisher()
    : Node("odom_to_pose_publisher")
    {
         // Get environment variable
        //const char* ns_env = std::getenv("ROM_ROBOT_NAMESPACE");
        //std::string ns = (ns_env != nullptr) ? std::string(ns_env) : "default_robot1";
         // Build topic names
        //std::string odom_topic = ns + "/odom";
        //std::string pose_topic = ns + "/amcl_pose";


        // Create a publisher for /robot_pose
        
        robot_pose_publisher_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>("amcl_pose", 10);


        // Create a subscriber for odom
        odom_subscriber_ = this->create_subscription<nav_msgs::msg::Odometry>(

            "odom", 10, std::bind(&OdomToPosePublisher::odom_callback, this, std::placeholders::_1));

    }

private:
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        // Create a PoseWithCovarianceStamped message
        geometry_msgs::msg::PoseWithCovarianceStamped pose_with_covariance_msg;

        // Set the header
        pose_with_covariance_msg.header.stamp = this->get_clock()->now();
        pose_with_covariance_msg.header.frame_id = "odom";  // You can change this to your desired frame

        // Set position and orientation from Odometry
        pose_with_covariance_msg.pose = msg->pose;

        // Publish the PoseWithCovarianceStamped message
        robot_pose_publisher_->publish(pose_with_covariance_msg);
    }

    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscriber_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr robot_pose_publisher_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomToPosePublisher>());
    rclcpp::shutdown();
    return 0;
}

