#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "nav_msgs/msg/path.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav2_multi_mode_navigation/msg/mission_path.hpp"

namespace nav2_multi_mode_navigation
{

class GetPathFromTopic : public BT::StatefulActionNode
{
public:
  GetPathFromTopic(
    const std::string & name,
    const BT::NodeConfiguration & config);

  static BT::PortsList providedPorts();

private:
  BT::NodeStatus onStart() override;
  BT::NodeStatus onRunning() override;
  void onHalted() override;
  void pathCallback(
    nav2_multi_mode_navigation::msg::MissionPath::ConstSharedPtr message);
  void missionIdCallback(std_msgs::msg::String::ConstSharedPtr message);

  rclcpp::Node::SharedPtr node_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::executors::SingleThreadedExecutor callback_group_executor_;
  rclcpp::Subscription<nav2_multi_mode_navigation::msg::MissionPath>::SharedPtr subscription_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr mission_id_subscription_;
  std::mutex mutex_;
  nav_msgs::msg::Path latest_path_;
  std::string latest_mission_id_;
  std::string active_mission_id_;
  bool received_{false};
  std::string configured_mission_id_;
  std::chrono::steady_clock::time_point deadline_;
  int timeout_ms_{5000};
  int max_age_ms_{3000};
};

}  // namespace nav2_multi_mode_navigation
