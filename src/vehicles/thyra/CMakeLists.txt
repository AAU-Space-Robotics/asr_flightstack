cmake_minimum_required(VERSION 3.5)
project(thyra)

# Default to C++20
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(asr_comms REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)

ament_export_dependencies(rosidl_default_runtime)

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
  FILES_MATCHING PATTERN "*.py"
)

# Install config files
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
  FILES_MATCHING PATTERN "*.yaml"
)


add_executable(thyra_mission_executor
  src/main.cpp
)
target_link_libraries(thyra_mission_executor
  Eigen3::Eigen
)

ament_target_dependencies(thyra_mission_executor
  rclcpp
  rclcpp_action
  std_msgs
  geometry_msgs
  asr_comms
  cv_bridge
)

install(TARGETS
  thyra_mission_executor
  DESTINATION lib/${PROJECT_NAME}
)


ament_package()
