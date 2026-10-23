#include "nav2_multi_mode_navigation/get_path_from_topic.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "behaviortree_cpp/bt_factory.h"

namespace nav2_multi_mode_navigation
{

GetPathFromTopic::GetPathFromTopic(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::StatefulActionNode(name, config)
{
  // This is the same blackboard key and node type used by Nav2's BT selector nodes.
  node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");

  std::string path_topic = "/mission_path";
  std::string mission_id_topic = "/active_mission_id";
  (void)getInput("topic", path_topic);
  (void)getInput("mission_id_topic", mission_id_topic);
  (void)getInput("timeout_ms", timeout_ms_);
  (void)getInput("max_age_ms", max_age_ms_);
  timeout_ms_ = std::max(1, timeout_ms_);
  max_age_ms_ = std::max(1, max_age_ms_);

  // Use a private callback group/executor so callback progress does not depend on
  // the BT Navigator's main executor spinning this subscription at the right time.
  callback_group_ = node_->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive, false);
  callback_group_executor_.add_callback_group(
    callback_group_, node_->get_node_base_interface());

  rclcpp::SubscriptionOptions options;
  options.callback_group = callback_group_;
  subscription_ = node_->create_subscription<
    nav2_multi_mode_navigation::msg::MissionPath>(
    path_topic, rclcpp::QoS(rclcpp::KeepLast(1)).reliable(),
    std::bind(&GetPathFromTopic::pathCallback, this, std::placeholders::_1),
    options);

  // Transient-local durability allows the BT to receive the current mission ID
  // even if the mission dispatcher published it just before the BT started.
  mission_id_subscription_ = node_->create_subscription<std_msgs::msg::String>(
    mission_id_topic, rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local(),
    std::bind(&GetPathFromTopic::missionIdCallback, this, std::placeholders::_1),
    options);
  callback_group_executor_.spin_some(std::chrono::nanoseconds(1));
}

BT::PortsList GetPathFromTopic::providedPorts()
{
  return {
    BT::InputPort<std::string>("mission_id", "", "Optional fixed mission ID; otherwise read mission_id_topic"),
    BT::InputPort<std::string>("mission_id_topic", "/active_mission_id", "Transient-local std_msgs/String topic"),
    BT::InputPort<std::string>("topic", "/mission_path", "MissionPath topic"),
    BT::InputPort<int>("timeout_ms", 5000, "Maximum wait for matching path"),
    BT::InputPort<int>("max_age_ms", 3000, "Maximum age of path.header.stamp"),
    BT::OutputPort<nav_msgs::msg::Path>("path", "Path for the Nav2 FollowPath action")
  };
}

BT::NodeStatus GetPathFromTopic::onStart()
{
  configured_mission_id_.clear();
  (void)getInput("mission_id", configured_mission_id_);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    received_ = false;
    latest_mission_id_.clear();
    latest_path_ = nav_msgs::msg::Path{};
    active_mission_id_.clear();
  }
  deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms_);
  return onRunning();
}

BT::NodeStatus GetPathFromTopic::onRunning()
{
  callback_group_executor_.spin_some();

  nav_msgs::msg::Path candidate;
  std::string expected_mission_id = configured_mission_id_;
  bool ready = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (expected_mission_id.empty()) {
      expected_mission_id = active_mission_id_;
    }
    ready = !expected_mission_id.empty() && received_ &&
      latest_mission_id_ == expected_mission_id;
    if (ready) {
      candidate = latest_path_;
    }
  }

  if (ready) {
    if (candidate.poses.empty()) {
      RCLCPP_ERROR(node_->get_logger(), "Received an empty path for mission '%s'",
        expected_mission_id.c_str());
      return BT::NodeStatus::FAILURE;
    }
    if (candidate.header.frame_id.empty()) {
      RCLCPP_ERROR(node_->get_logger(), "Path for mission '%s' has an empty frame_id",
        expected_mission_id.c_str());
      return BT::NodeStatus::FAILURE;
    }
    if (candidate.header.stamp.sec == 0 && candidate.header.stamp.nanosec == 0) {
      RCLCPP_WARN(node_->get_logger(), "Path has a zero timestamp; rejecting untimestamped input");
      return BT::NodeStatus::FAILURE;
    }

    const auto stamp = rclcpp::Time(candidate.header.stamp, node_->get_clock()->get_clock_type());
    const auto age = (node_->now() - stamp).nanoseconds();
    if (age < 0 || age > static_cast<int64_t>(max_age_ms_) * 1000000LL) {
      RCLCPP_WARN(node_->get_logger(), "Path for mission '%s' is stale or has a future timestamp",
        expected_mission_id.c_str());
      return BT::NodeStatus::FAILURE;
    }

    setOutput("path", candidate);
    return BT::NodeStatus::SUCCESS;
  }

  if (std::chrono::steady_clock::now() >= deadline_) {
    RCLCPP_ERROR(node_->get_logger(), "Timed out waiting for a current mission ID and matching path");
    return BT::NodeStatus::FAILURE;
  }
  return BT::NodeStatus::RUNNING;
}

void GetPathFromTopic::onHalted()
{
  // No blocking wait is used. The subscription remains active for the next mission.
}

void GetPathFromTopic::pathCallback(
  nav2_multi_mode_navigation::msg::MissionPath::ConstSharedPtr message)
{
  std::lock_guard<std::mutex> lock(mutex_);
  latest_mission_id_ = message->mission_id;
  latest_path_ = message->path;
  received_ = true;
}

void GetPathFromTopic::missionIdCallback(std_msgs::msg::String::ConstSharedPtr message)
{
  std::lock_guard<std::mutex> lock(mutex_);
  active_mission_id_ = message->data;
}

}  // namespace nav2_multi_mode_navigation

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nav2_multi_mode_navigation::GetPathFromTopic>("GetPathFromTopic");
}
