cmake_minimum_required(VERSION 3.8)
project(nav2_multi_mode_navigation)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/MissionPath.msg"
  DEPENDENCIES nav_msgs
)

ament_export_dependencies(rosidl_default_runtime)

add_library(${PROJECT_NAME}_bt_node SHARED
  src/get_path_from_topic.cpp
)
target_compile_features(${PROJECT_NAME}_bt_node PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME}_bt_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(${PROJECT_NAME}_bt_node
  rclcpp nav_msgs std_msgs behaviortree_cpp
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(${PROJECT_NAME}_bt_node "${cpp_typesupport_target}")

install(TARGETS ${PROJECT_NAME}_bt_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY behavior_trees config launch DESTINATION share/${PROJECT_NAME})
install(PROGRAMS scripts/send_navigation_goal.py DESTINATION lib/${PROJECT_NAME})

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME}_bt_node)
ament_package()
