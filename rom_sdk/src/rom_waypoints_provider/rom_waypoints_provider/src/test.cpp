#include "rclcpp/rclcpp.hpp"
#include "yaml-cpp/yaml.h"

class YamlReader : public rclcpp::Node
{
public:
    YamlReader() : Node("yaml_reader")
    {
        // Load the YAML file
        try
        {
            // Use YAML::LoadFile to load the YAML file
            YAML::Node config = YAML::LoadFile("config.yaml");

            
            if (config["robot_name"])
            {
                std::string robot_name = config["robot_name"].as<std::string>();
                RCLCPP_INFO(this->get_logger(), "Robot Name: %s", robot_name.c_str());
            }
            else
            {
                RCLCPP_WARN(this->get_logger(), "No 'robot_name' found in the YAML file");
            }

            if (config["robot_speed"])
            {
                double robot_speed = config["robot_speed"].as<double>();
                RCLCPP_INFO(this->get_logger(), "Robot Speed: %f", robot_speed);
            }
            else
            {
                RCLCPP_WARN(this->get_logger(), "No 'robot_speed' found in the YAML file");
            }
        }
        catch (const YAML::Exception &e)
        {
            RCLCPP_ERROR(this->get_logger(), "Error loading YAML file: %s", e.what());
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<YamlReader>());
    rclcpp::shutdown();
    return 0;
}
