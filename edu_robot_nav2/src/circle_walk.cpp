#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <random>

class RandomWalkNode : public rclcpp::Node
{
public:
    RandomWalkNode() : Node("random_walk_node")
    {
        // Create a publisher to /cmd_vel
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_joy", 10);

        
        // Create a timer to publish random velocities at a regular interval (e.g., 100 ms)
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50), std::bind(&RandomWalkNode::random_walk_callback, this));
    }

private:

    void random_walk_callback()
    {
        // Create a new Twist message
        auto twist = geometry_msgs::msg::Twist();

        
        twist.linear.x  = 0.5;
        twist.angular.z = 0.2;

        // Publish the velocity command
        publisher_->publish(twist);
    }

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    // Random number generation
    std::random_device rd_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> linear_dist_;
    std::uniform_real_distribution<double> angular_dist_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RandomWalkNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
