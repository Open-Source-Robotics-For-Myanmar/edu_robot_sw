#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <random>

class RandomWalkNode : public rclcpp::Node
{
public:
    RandomWalkNode() : Node("random_walk_node")
    {
        // Create a publisher to /cmd_vel
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel_joy", 20);
        count_ = 0;
        // Initialize random number generator
        rng_ = std::mt19937(rd_());
        linear_dist_ = std::uniform_real_distribution<double>(0.0, 0.5);
        angular_dist_ = std::uniform_real_distribution<double>(-1.0, 1.0);

        // Create a timer to publish random velocities at a regular interval (e.g., 100 ms)
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100), std::bind(&RandomWalkNode::random_walk_callback, this));
    }

private:

    void random_walk_callback()
    {
        count_ += 1;
        static double heading = 0; 
        // Create a new Twist message
        auto twist = geometry_msgs::msg::Twist();

        // Generate random linear and angular velocities
        twist.linear.x = linear_dist_(rng_);  // Random linear velocity between 0.0 and 0.5 m/s
        
        if(count_ >= 20) {
            heading = angular_dist_(rng_); 
            count_ = 0; 
        }
        twist.angular.z = heading;

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
    short count_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RandomWalkNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
