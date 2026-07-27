cmake_minimum_required(VERSION 3.10)
project(asr_mission)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(asr_comms REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)

# --- asr_mission library: plan schema, capabilities, validator, executor ---
# ROS-free (no rclcpp types in these headers) so it can be linked by any
# consumer -- the onboard node here, and the C++ GCS (asr_gcs) later -- with
# no bridge between them.
add_library(asr_mission_lib
  src/plan.cpp
  src/capabilities.cpp
  src/plan_validator.cpp
  src/plan_executor.cpp
)
target_include_directories(asr_mission_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(asr_mission_lib
  nlohmann_json::nlohmann_json
  yaml-cpp::yaml-cpp
  OpenSSL::Crypto
)

# --- mission_executor: onboard ROS node wrapping PlanExecutor ---
add_executable(mission_executor
  src/mission_executor_node.cpp
)
target_link_libraries(mission_executor
  asr_mission_lib
)
ament_target_dependencies(mission_executor
  rclcpp
  rclcpp_action
  std_msgs
  asr_comms
)

# --- mission_cli: disposable GCS-side test tool (see tools/mission_cli.cpp) ---
add_executable(mission_cli
  tools/mission_cli.cpp
)
target_link_libraries(mission_cli
  asr_mission_lib
)
ament_target_dependencies(mission_cli
  rclcpp
  std_msgs
)

install(
  TARGETS asr_mission_lib
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS mission_executor mission_cli
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/
  DESTINATION include
)
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)
install(DIRECTORY plans/
  DESTINATION share/${PROJECT_NAME}/plans
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(nlohmann_json yaml-cpp)
ament_export_include_directories(include)

ament_package()
