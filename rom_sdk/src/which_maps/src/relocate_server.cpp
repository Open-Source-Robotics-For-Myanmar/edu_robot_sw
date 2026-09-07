/**
 * relocate_server.cpp
 *
 * Subscribes to /<ns>/relocate (geometry_msgs/PoseWithCovarianceStamped),
 * which is published by the Qt GUI via rosbridge, and performs a Cartographer
 * hard reset at the requested pose:
 *
 *   1. GetTrajectoryStates  ─ find the current ACTIVE trajectory ID
 *   2. FinishTrajectory     ─ gracefully end that trajectory
 *   3. StartTrajectory      ─ start a fresh one from the supplied pose
 *
 * The subscribe callback immediately returns (to avoid blocking the executor)
 * and offloads all blocking service calls to a worker std::thread.
 * The MultiThreadedExecutor processes service responses (svc_cb_group_)
 * concurrently, resolving the std::shared_futures the worker thread waits on.
 *
 * Environment variables (all optional):
 *   ROM_ROBOT_NAMESPACE     robot namespace (default: "")
 *   ROM_ROBOT_MODEL         robot model name (default: "bobo")
 *   CARTO_CONFIG_DIR        full path to the Cartographer .lua config dir
 *   CARTO_CONFIG_BASENAME   .lua filename  (default: <model>_nav_2d.lua)
 */

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "ament_index_cpp/get_package_share_directory.hpp"
#include "cartographer_ros_msgs/msg/trajectory_states.hpp"
#include "cartographer_ros_msgs/srv/finish_trajectory.hpp"
#include "cartographer_ros_msgs/srv/get_trajectory_states.hpp"
#include "cartographer_ros_msgs/srv/start_trajectory.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "rclcpp/rclcpp.hpp"

// ── Type aliases ────────────────────────────────────────────────────────────
using FinishTraj     = cartographer_ros_msgs::srv::FinishTrajectory;
using GetStates      = cartographer_ros_msgs::srv::GetTrajectoryStates;
using StartTraj      = cartographer_ros_msgs::srv::StartTrajectory;
using TrajectoryStates = cartographer_ros_msgs::msg::TrajectoryStates;
using PoseWCS        = geometry_msgs::msg::PoseWithCovarianceStamped;

static constexpr std::chrono::seconds SVC_WAIT_TIMEOUT{3};
static constexpr std::chrono::seconds SVC_CALL_TIMEOUT{10};

// ────────────────────────────────────────────────────────────────────────────

class RelocateServer : public rclcpp::Node
{
public:
  RelocateServer()
  : Node("relocate_server"), is_relocating_(false)
  {
    // ── Read environment variables ───────────────────────────────────────
    const char * ns_env       = std::getenv("ROM_ROBOT_NAMESPACE");
    const char * model_env    = std::getenv("ROM_ROBOT_MODEL");
    const char * cfg_dir_env  = std::getenv("CARTO_CONFIG_DIR");
    const char * cfg_base_env = std::getenv("CARTO_CONFIG_BASENAME");

    robot_namespace_ = ns_env    ? ns_env    : "";
    robot_model_     = model_env ? model_env : "bobo";

    // Config directory: env var → ament_index lookup → hardcoded fallback
    if (cfg_dir_env) {
      carto_config_dir_ = cfg_dir_env;
    } else {
      try {
        carto_config_dir_ =
          ament_index_cpp::get_package_share_directory(robot_model_ + "_carto")
          + "/config/";
      } catch (const std::exception &) {
        // Workspace for this package may not be sourced; use known install path
        carto_config_dir_ =
          "/home/buc_robot/rom_nav2_ws/install/" + robot_model_ +
          "_carto/share/" + robot_model_ + "_carto/config/";
        RCLCPP_WARN(get_logger(),
          "[RelocateServer] ament_index lookup failed for '%s_carto'. "
          "Falling back to: %s",
          robot_model_.c_str(), carto_config_dir_.c_str());
      }
    }
    carto_config_base_ =
      cfg_base_env ? cfg_base_env : (robot_model_ + "_nav_2d.lua");

    // ── Service / topic names (namespace-aware) ──────────────────────────
    // Empty namespace → /finish_trajectory
    // "robot1"        → /robot1/finish_trajectory
    const std::string ns_prefix  =
      robot_namespace_.empty() ? "" : "/" + robot_namespace_;

    const std::string states_srv = ns_prefix + "/get_trajectory_states";
    const std::string finish_srv = ns_prefix + "/finish_trajectory";
    const std::string start_srv  = ns_prefix + "/start_trajectory";
    const std::string rel_topic  = ns_prefix + "/relocate";

    // ── Callback groups ──────────────────────────────────────────────────
    // Subscriber and service clients live in separate MutuallyExclusive groups.
    // MultiThreadedExecutor can therefore process service responses (svc_cb_group_)
    // while the subscriber group is held by the callback or worker thread.
    sub_cb_group_ =
      create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    svc_cb_group_ =
      create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

    // ── Service clients ──────────────────────────────────────────────────
    states_client_ = create_client<GetStates>(
      states_srv, rmw_qos_profile_services_default, svc_cb_group_);
    finish_client_ = create_client<FinishTraj>(
      finish_srv, rmw_qos_profile_services_default, svc_cb_group_);
    start_client_  = create_client<StartTraj>(
      start_srv, rmw_qos_profile_services_default, svc_cb_group_);

    // ── Subscriber ───────────────────────────────────────────────────────
    rclcpp::SubscriptionOptions sub_opts;
    sub_opts.callback_group = sub_cb_group_;
    subscription_ = create_subscription<PoseWCS>(
      rel_topic, 10,
      std::bind(&RelocateServer::onRelocate, this, std::placeholders::_1),
      sub_opts);

    RCLCPP_INFO(get_logger(),
      "[RelocateServer] Ready. topic='%s'  config='%s%s'",
      rel_topic.c_str(), carto_config_dir_.c_str(), carto_config_base_.c_str());
  }

private:
  // ── Subscriber callback ─────────────────────────────────────────────────
  // Returns immediately; the blocking reset work is done in a worker thread.
  void onRelocate(const PoseWCS::SharedPtr msg)
  {
    bool expected = false;
    if (!is_relocating_.compare_exchange_strong(expected, true)) {
      RCLCPP_WARN(get_logger(),
        "[RelocateServer] Previous relocation still in progress — ignored.");
      return;
    }
    std::thread(&RelocateServer::doRelocation, this, msg).detach();
  }

  // ── Worker: Cartographer hard reset ─────────────────────────────────────
  void doRelocation(const PoseWCS::SharedPtr msg)
  {
    RCLCPP_INFO(get_logger(),
      "[RelocateServer] Hard reset: x=%.3f  y=%.3f  yaw=%.3f rad",
      msg->pose.pose.position.x,
      msg->pose.pose.position.y,
      quaternionToYaw(msg->pose.pose.orientation));

    // ── Step 1: identify the currently ACTIVE trajectory ────────────────
    int32_t active_id = -1;
    if (!getActiveTrajectoryId(active_id)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] Cannot determine active trajectory. Aborting.");
      is_relocating_ = false;
      return;
    }
    RCLCPP_INFO(get_logger(),
      "[RelocateServer] Active trajectory ID = %d", active_id);

    // ── Step 2: finish that trajectory ──────────────────────────────────
    if (!callFinishTrajectory(active_id)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] finish_trajectory(%d) failed. Aborting.", active_id);
      is_relocating_ = false;
      return;
    }

    // ── Step 3: start a new trajectory from the requested pose ──────────
    if (!callStartTrajectory(msg->pose.pose, active_id)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] start_trajectory failed. Robot localization lost!");
      is_relocating_ = false;
      return;
    }

    RCLCPP_INFO(get_logger(), "[RelocateServer] Relocation complete.");
    is_relocating_ = false;
  }

  // ── Helper: GetTrajectoryStates → first ACTIVE id ───────────────────────
  bool getActiveTrajectoryId(int32_t & out_id)
  {
    if (!states_client_->wait_for_service(SVC_WAIT_TIMEOUT)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] get_trajectory_states service not available.");
      return false;
    }

    auto future =
      states_client_->async_send_request(std::make_shared<GetStates::Request>());

    // The MultiThreadedExecutor (svc_cb_group_) resolves this future; we just wait.
    if (future.wait_for(SVC_CALL_TIMEOUT) != std::future_status::ready) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] get_trajectory_states timed out.");
      return false;
    }

    auto resp = future.get();
    if (resp->status.code != 0) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] get_trajectory_states error: %s",
        resp->status.message.c_str());
      return false;
    }

    const auto & ts = resp->trajectory_states;
    for (size_t i = 0; i < ts.trajectory_id.size(); ++i) {
      if (ts.trajectory_state[i] == TrajectoryStates::ACTIVE) {
        out_id = ts.trajectory_id[i];
        return true;
      }
    }

    RCLCPP_ERROR(get_logger(),
      "[RelocateServer] No ACTIVE trajectory found among %zu trajectories.",
      ts.trajectory_id.size());
    return false;
  }

  // ── Helper: FinishTrajectory ─────────────────────────────────────────────
  bool callFinishTrajectory(int32_t trajectory_id)
  {
    if (!finish_client_->wait_for_service(SVC_WAIT_TIMEOUT)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] finish_trajectory service not available.");
      return false;
    }

    auto req          = std::make_shared<FinishTraj::Request>();
    req->trajectory_id = trajectory_id;

    auto future = finish_client_->async_send_request(req);
    if (future.wait_for(SVC_CALL_TIMEOUT) != std::future_status::ready) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] finish_trajectory(%d) timed out.", trajectory_id);
      return false;
    }

    auto resp = future.get();
    if (resp->status.code != 0) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] finish_trajectory(%d) error: %s",
        trajectory_id, resp->status.message.c_str());
      return false;
    }

    RCLCPP_INFO(get_logger(),
      "[RelocateServer] Trajectory %d finished OK.", trajectory_id);
    return true;
  }

  // ── Helper: StartTrajectory with initial pose ────────────────────────────
  bool callStartTrajectory(
    const geometry_msgs::msg::Pose & pose,
    int32_t relative_to_id)
  {
    if (!start_client_->wait_for_service(SVC_WAIT_TIMEOUT)) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] start_trajectory service not available.");
      return false;
    }

    auto req                          = std::make_shared<StartTraj::Request>();
    req->configuration_directory      = carto_config_dir_;
    req->configuration_basename       = carto_config_base_;
    req->use_initial_pose             = true;
    req->initial_pose                 = pose;       // geometry_msgs/Pose
    req->relative_to_trajectory_id    = relative_to_id;

    auto future = start_client_->async_send_request(req);
    if (future.wait_for(SVC_CALL_TIMEOUT) != std::future_status::ready) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] start_trajectory timed out.");
      return false;
    }

    auto resp = future.get();
    if (resp->status.code != 0) {
      RCLCPP_ERROR(get_logger(),
        "[RelocateServer] start_trajectory error: %s",
        resp->status.message.c_str());
      return false;
    }

    RCLCPP_INFO(get_logger(),
      "[RelocateServer] New trajectory %d started at x=%.3f y=%.3f.",
      resp->trajectory_id, pose.position.x, pose.position.y);
    return true;
  }

  // ── Utility ──────────────────────────────────────────────────────────────
  static double quaternionToYaw(const geometry_msgs::msg::Quaternion & q)
  {
    // yaw = atan2( 2*(w*z + x*y), 1 - 2*(y² + z²) )
    const double siny = 2.0 * (q.w * q.z + q.x * q.y);
    const double cosy = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    return std::atan2(siny, cosy);
  }

  // ── Member variables ──────────────────────────────────────────────────────
  rclcpp::Subscription<PoseWCS>::SharedPtr subscription_;
  rclcpp::Client<GetStates>::SharedPtr     states_client_;
  rclcpp::Client<FinishTraj>::SharedPtr    finish_client_;
  rclcpp::Client<StartTraj>::SharedPtr     start_client_;
  rclcpp::CallbackGroup::SharedPtr         sub_cb_group_;
  rclcpp::CallbackGroup::SharedPtr         svc_cb_group_;

  std::string robot_namespace_;
  std::string robot_model_;
  std::string carto_config_dir_;
  std::string carto_config_base_;

  // Prevents a second relocation from starting before the first finishes
  std::atomic<bool> is_relocating_;
};

// ────────────────────────────────────────────────────────────────────────────

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<RelocateServer>();

  // 4 threads: ensures sub_cb_group_ and svc_cb_group_ can run concurrently
  // so that service responses (needed by the worker thread's wait_for()) are
  // processed without waiting for the subscriber callback group to free.
  rclcpp::executors::MultiThreadedExecutor executor(
    rclcpp::ExecutorOptions(), 4);
  executor.add_node(node);
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
