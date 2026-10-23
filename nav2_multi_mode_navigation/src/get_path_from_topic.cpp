#include "nav2_multi_mode_navigation/get_path_from_topic.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

#include "behaviortree_cpp/bt_factory.h"

namespace nav2_multi_mode_navigation
{

GetPathFromTopic::GetPathFromTopic(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::StatefulActionNode(name, config)
{
  // Nav2's BT blackboard exposes the ROS node under the "node" key.
  node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");

  std::string topic = "/mission_path";
  (void)getInput("topic", topic);
  (void)getInput("timeout_ms", timeout_ms_);
  (void)getInput("max_age_ms", max_age_ms_);
  timeout_ms_ = std::max(1, timeout_ms_);
  max_age_ms_ = std::max(1, max_age_ms_);

  subscription_ = node_->create_subscription<
    nav2_multi_mode_navigation::msg::MissionPath>(
    topic, rclcpp::QoS(1).reliable(),
    std::bind(&GetPathFromTopic::pathCallback, this, std::placeholders::_1));
}

BT::PortsList GetPathFromTopic::providedPorts()
{
  return {
    BT::InputPort<std::string>("mission_id", "ID of the active mission"),
    BT::InputPort<std::string>("topic", "/mission_path", "MissionPath topic"),
    BT::InputPort<int>("timeout_ms", 5000, "Maximum wait for matching path"),
    BT::InputPort<int>("max_age_ms", 3000, "Maximum age of path.header.stamp"),
    BT::OutputPort<nav_msgs::msg::Path>("path", "Path for the Nav2 FollowPath action")
  };
}

BT::NodeStatus GetPathFromTopic::onStart()
{
  if (!getInput("mission_id", expected_mission_id_) || expected_mission_id_.empty()) {
    RCLCPP_ERROR(node_->get_logger(), "GetPathFromTopic requires a non-empty mission_id");
    return BT::NodeStatus::FAILURE;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    received_ = false;
    latest_mission_id_.clear();
    latest_path_ = nav_msgs::msg::Path{};
  }
  deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms_);
  return onRunning();
}

BT::NodeStatus GetPathFromTopic::onRunning()
{
  nav_msgs::msg::Path candidate;
  bool ready = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ready = received_ && latest_mission_id_ == expected_mission_id_;
    if (ready) {
      candidate = latest_path_;
    }
  }

  if (ready) {
    if (candidate.poses.empty()) {
      RCLCPP_ERROR(node_->get_logger(), "Received an empty path for mission '%s'",
        expected_mission_id_.c_str());
      return BT::NodeStatus::FAILURE;
    }

    const auto stamp = rclcpp::Time(candidate.header.stamp);
    if (candidate.header.stamp.sec == 0 && candidate.header.stamp.nanosec == 0) {
      RCLCPP_WARN(node_->get_logger(), "Path has a zero timestamp; rejecting stale/untimestamped input");
      return BT::NodeStatus::FAILURE;
    }
    const auto age = (node_->now() - stamp).nanoseconds();
    if (age < 0 || age > static_cast<int64_t>(max_age_ms_) * 1000000LL) {
      RCLCPP_WARN(node_->get_logger(), "Path for mission '%s' is stale or has a future timestamp",
        expected_mission_id_.c_str());
      return BT::NodeStatus::FAILURE;
    }

    setOutput("path", candidate);
    return BT::NodeStatus::SUCCESS;
  }

  if (std::chrono::steady_clock::now() >= deadline_) {
    RCLCPP_ERROR(node_->get_logger(), "Timed out waiting for path for mission '%s'",
      expected_mission_id_.c_str());
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

}  // namespace nav2_multi_mode_navigation

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nav2_multi_mode_navigation::GetPathFromTopic>("GetPathFromTopic");
}
