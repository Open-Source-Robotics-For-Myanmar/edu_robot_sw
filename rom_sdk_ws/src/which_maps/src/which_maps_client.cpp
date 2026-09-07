#include "rclcpp/rclcpp.hpp"
#include "rom_interfaces/srv/which_maps.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>

using namespace std::chrono_literals;

// INPUT ( which_map_do_you_have | save_map | select_map | mapping | navi | remapping )

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("which_map_client");
  
  rclcpp::Client<rom_interfaces::srv::WhichMaps>::SharedPtr client = node->create_client<rom_interfaces::srv::WhichMaps>("which_maps");

  auto request = std::make_shared<rom_interfaces::srv::WhichMaps::Request>();

  
  // // Wait for "which_maps" service
  while (!client->wait_for_service(1s)) {
    if (!rclcpp::ok()) {
      RCLCPP_ERROR(rclcpp::get_logger("which_maps_client"), "Interrupted while waiting for the service. Exiting.");
      return 0;
    }
    RCLCPP_INFO(rclcpp::get_logger("which_maps_client"), "Service not available, waiting again...");
  }

  auto send_request_and_wait = [&](const std::string& request_string, const std::string& optional_param = "") {
        request->request_string = request_string;

        if (request_string == "save_map") {
            request->map_name_to_save = optional_param;
        } else if (request_string == "select_map") {
            request->map_name_to_select = optional_param;
        }

        auto result_future = client->async_send_request(request);

        if (rclcpp::spin_until_future_complete(node, result_future) == rclcpp::FutureReturnCode::SUCCESS) {
            auto response = result_future.get();
            RCLCPP_INFO(rclcpp::get_logger("which_maps_client"), "%s response status: %d", request_string.c_str(), response->status);

            if (response->status == 1) {
                RCLCPP_INFO(rclcpp::get_logger("which_maps_client"), "%s response OK", request_string.c_str());
            } else {
                RCLCPP_WARN(rclcpp::get_logger("which_maps_client"), "%s response NOT OK", request_string.c_str());
            }
        } else {
            RCLCPP_ERROR(rclcpp::get_logger("which_maps_client"), "Failed to call service for %s", request_string.c_str());
        }
    };

    // Make requests with the helper function
    send_request_and_wait("which_map_do_you_have");
    send_request_and_wait("save_map", "default_map");
    send_request_and_wait("select_map", "default_map");
    send_request_and_wait("mapping");
    send_request_and_wait("navi");
    send_request_and_wait("remapping");

  rclcpp::shutdown();
  return 0;
}