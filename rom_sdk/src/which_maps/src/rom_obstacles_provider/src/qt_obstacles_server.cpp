#define IS_PRODUCTION 1

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <geometry_msgs/msg/point.hpp>
#include "rom_interfaces/srv/add_obstacles.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace std::chrono_literals;

const std::string yaml_path = "/home/mr_robot/data/obstacles/line_obstacles.yaml";
const char *ns_env = std::getenv("ROM_ROBOT_NAMESPACE");
const std::string rom_robot_namespace = (ns_env != nullptr) ? ns_env : "";

class LineObstaclePublisher : public rclcpp::Node
{
public:
  LineObstaclePublisher(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("line_obstacle_publisher", rom_robot_namespace, options)
  {
    publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("line_obstacles", 1);
    
    service_ = this->create_service<rom_interfaces::srv::AddObstacles>(
      "add_line_obstacles",
      std::bind(&LineObstaclePublisher::handle_add_obstacles, this, 
                std::placeholders::_1, std::placeholders::_2));
    // timer_ = this->create_wall_timer(1s, std::bind(&LineObstaclePublisher::timer_callback, this));
    // Production နှင့် Simulation ပေါ်မူတည်ပြီး Timer ခွဲခြားခြင်း
    #ifdef IS_PRODUCTION
        // စက်ရုပ်အစစ်အတွက်: တကယ့်နံရံကပ်နာရီ အချိန်အတိုင်း Run မည်
        timer_ = this->create_wall_timer(1s, std::bind(&LineObstaclePublisher::timer_callback, this));
    #else
        // Simulation အတွက်: rclcpp::create_timer ကိုပြောင်းသုံးပြီး error ကိုရှင်းခြင်း
        timer_ = rclcpp::create_timer(
            this, // Node ရဲ့ Context ကို လွှဲပေးရန်
            this->get_clock(),
            1s,
            std::bind(&LineObstaclePublisher::timer_callback, this)
        );
    #endif
      
    RCLCPP_INFO(this->get_logger(), "Line Obstacle Publisher Node Started.");
    RCLCPP_INFO(this->get_logger(), "Service 'add_line_obstacles' is ready.");
  }

private:
  void handle_add_obstacles(
    const std::shared_ptr<rom_interfaces::srv::AddObstacles::Request> request,
    std::shared_ptr<rom_interfaces::srv::AddObstacles::Response> response)
  {
    // Default response status
    response->status = -1;

    if( request->mode == "add_obs_lines" )
    {
        // စစ်ဆေးခြင်း: start_points vector size ရှိမရှိ
        if (request->start_points.empty() || request->end_points.empty()) 
        {
        RCLCPP_WARN(this->get_logger(), "Received empty start_points or end_points. No obstacles will be published.");
        return;
        }

        // စစ်ဆေးခြင်း: start_points နှင့် end_points size တူညီမှုရှိမရှိ
        if (request->start_points.size() != request->end_points.size()) 
        {
        RCLCPP_ERROR(this->get_logger(), "start_points and end_points size mismatch!");
        return;
        }

        // စစ်ဆေးခြင်း: scene points များရှိမရှိ
        if (request->scene_start_points.size() != request->start_points.size() ||
            request->scene_end_points.size() != request->end_points.size()) 
        {
        RCLCPP_ERROR(this->get_logger(), "scene points size mismatch with map points!");
        return;
        }

        RCLCPP_INFO(this->get_logger(), "Received %zu line obstacles from service request.", 
                    request->start_points.size());

        // obstacle points ကို clear လုပ်ပြီး အသစ်သိမ်းဆည်းမည်
        obstacle_points_.clear();
        frame_id_ = request->frame_id.empty() ? "map" : request->frame_id;

        // start_points နှင့် end_points ကြားမှ line များကို generate လုပ်ခြင်း
        for (size_t i = 0; i < request->start_points.size(); ++i) 
        {
          const auto& start = request->start_points[i];
          const auto& end = request->end_points[i];
          
          // Line ပေါ်မှာ points များကို interpolate လုပ်ခြင်း
          generate_line_points(start, end);
        }

        // YAML file ထဲတွင် သိမ်းဆည်းခြင်း
        bool save_success = save_obstacles_to_yaml(request);
        
        if (save_success && !obstacle_points_.empty()) 
        {
          response->status = 1;
          RCLCPP_INFO(this->get_logger(), "Successfully saved %zu line obstacles with %zu total points.",
                      request->start_points.size(), obstacle_points_.size());
        } 
        else 
        {
          response->status = -1;
          RCLCPP_ERROR(this->get_logger(), "Failed to save obstacles or no points generated.");
        }
    }
    else if ( request->mode == "get_obs_lines")
    {

    }
    else if ( request->mode == "clear_obs_lines")
    {

    }
    else 
    {
        RCLCPP_ERROR(this->get_logger(), "Unknown mode in AddObstacles request: %s", request->mode.c_str());
        return;
    }

  }

  void generate_line_points(const geometry_msgs::msg::Point& start, 
                           const geometry_msgs::msg::Point& end)
  {
    // Line length တွက်ချက်ခြင်း
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double dz = end.z - start.z;
    double length = std::sqrt(dx*dx + dy*dy + dz*dz);

    // Points များကို 0.05 မီတာ spacing ဖြင့် generate လုပ်ခြင်း
    double spacing = 0.05; // 5 cm spacing
    int num_points = static_cast<int>(length / spacing) + 1;

    for (int i = 0; i <= num_points; ++i) {
      double t = (num_points > 0) ? static_cast<double>(i) / num_points : 0.0;
      std::array<double, 3> point = {
        start.x + t * dx,
        start.y + t * dy,
        start.z + t * dz
      };
      obstacle_points_.push_back(point);
    }
  }

  bool save_obstacles_to_yaml(
    const std::shared_ptr<rom_interfaces::srv::AddObstacles::Request> request)
  {

    // Directory ရှိမရှိစစ်ဆေးပြီး မရှိရင် ဖန်တီးခြင်း
    std::filesystem::path dir_path = std::filesystem::path(yaml_path).parent_path();
    if (!std::filesystem::exists(dir_path)) {
      std::filesystem::create_directories(dir_path);
      RCLCPP_INFO(this->get_logger(), "Created directory: %s", dir_path.c_str());
    }

    // YAML file ကို ဖွင့်ခြင်း (overwrite mode)
    std::ofstream yaml_file(yaml_path, std::ios::trunc);
    if (!yaml_file.is_open()) {
      RCLCPP_ERROR(this->get_logger(), "Failed to open YAML file: %s", yaml_path.c_str());
      return false;
    }

    // YAML format ဖြင့် ရေးသားခြင်း
    yaml_file << "# Line Obstacles Data\n";
    yaml_file << "# Generated by line_obstacle_publisher node\n";
    yaml_file << "frame_id: " << request->frame_id << "\n";
    yaml_file << "lines:\n";

    for (size_t i = 0; i < request->start_points.size(); ++i) {
      yaml_file << "  - line_" << i << ":\n";
      
      // Map frame coordinates (meters)
      yaml_file << "      start_point:\n";
      yaml_file << "        x: " << request->start_points[i].x << "\n";
      yaml_file << "        y: " << request->start_points[i].y << "\n";
      yaml_file << "        z: " << request->start_points[i].z << "\n";
      
      yaml_file << "      end_point:\n";
      yaml_file << "        x: " << request->end_points[i].x << "\n";
      yaml_file << "        y: " << request->end_points[i].y << "\n";
      yaml_file << "        z: " << request->end_points[i].z << "\n";
      
      // Scene frame coordinates (pixels)
      yaml_file << "      scene_start_point:\n";
      yaml_file << "        x: " << request->scene_start_points[i].x << "\n";
      yaml_file << "        y: " << request->scene_start_points[i].y << "\n";
      yaml_file << "        z: " << request->scene_start_points[i].z << "\n";
      
      yaml_file << "      scene_end_point:\n";
      yaml_file << "        x: " << request->scene_end_points[i].x << "\n";
      yaml_file << "        y: " << request->scene_end_points[i].y << "\n";
      yaml_file << "        z: " << request->scene_end_points[i].z << "\n";
    }

    yaml_file.close();
    RCLCPP_INFO(this->get_logger(), "Saved obstacles to: %s", yaml_path.c_str());
    return true;
  }

  void timer_callback()
  {
    // Obstacles များမရှိရင် publish မလုပ်ပါ
    if (obstacle_points_.empty()) {
      return;
    }

    auto cloud_msg = std::make_unique<sensor_msgs::msg::PointCloud2>();
    
    // Header ကို သတ်မှတ်ခြင်း
    cloud_msg->header.stamp = this->now();
    cloud_msg->header.frame_id = frame_id_; // frame_id from service request
    
    // PointCloud2 ၏ အခြေခံဖွဲ့စည်းပုံ သတ်မှတ်ခြင်း
    cloud_msg->height = 1;
    cloud_msg->width = obstacle_points_.size();
    cloud_msg->is_dense = true;
    cloud_msg->point_step = 12; // x(4) + y(4) + z(4) = 12 bytes
    cloud_msg->row_step = cloud_msg->point_step * cloud_msg->width;
    cloud_msg->data.resize(cloud_msg->row_step);
    
    // Field များကို သတ်မှတ်ခြင်း (x, y, z)
    sensor_msgs::PointCloud2Modifier modifier(*cloud_msg);
    modifier.setPointCloud2Fields(
        3, 
        "x", 1, sensor_msgs::msg::PointField::FLOAT32,
        "y", 1, sensor_msgs::msg::PointField::FLOAT32,
        "z", 1, sensor_msgs::msg::PointField::FLOAT32
    );

    // Points များကို PointCloud2 ၏ byte array ထဲသို့ ထည့်သွင်းခြင်း
    sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
    sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
    sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");
    
    for (const auto& p : obstacle_points_) {
      *iter_x = static_cast<float>(p[0]);
      *iter_y = static_cast<float>(p[1]);
      *iter_z = static_cast<float>(p[2]);
      ++iter_x;
      ++iter_y;
      ++iter_z;
    }

    // Message ကို publish လုပ်ခြင်း
    publisher_->publish(std::move(cloud_msg));
    RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                        "Publishing %zu obstacle points on topic 'line_obstacles'", 
                        obstacle_points_.size());
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
  rclcpp::Service<rom_interfaces::srv::AddObstacles>::SharedPtr service_;
  std::vector<std::array<double, 3>> obstacle_points_; // (x, y, z)
  std::string frame_id_{"map"}; // default frame_id
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // Constructor ထဲသို့ rclcpp::NodeOptions() ကို ပို့ပေးလိုက်ခြင်း
  rclcpp::spin(std::make_shared<LineObstaclePublisher>(rclcpp::NodeOptions()));
  rclcpp::shutdown();
  return 0;
}