cmake_minimum_required(VERSION 3.5)

# ------------------------------------------------------------------------------
project(asr_autopilot)

# Default to C++20
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# Compiler options
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(asr_comms REQUIRED)

# Include directories
include_directories(include)

add_library(asr_autopilot_core
  src/state_manager.cpp
  src/transformations.cpp
)

ament_target_dependencies(asr_autopilot_core
  rclcpp
  asr_comms
  px4_msgs
)

target_include_directories(asr_autopilot_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
# Create the C++ executable
add_executable(asr_autopilot src/main.cpp src/controller.cpp src/path_planner.cpp)


target_link_libraries(asr_autopilot asr_autopilot_core) 

# Add dependencies for asr_autopilot executable
ament_target_dependencies(asr_autopilot
  rclcpp
  rclcpp_action
  asr_comms
  px4_msgs
)

# Install C++ executables
install(TARGETS
  asr_autopilot
  DESTINATION lib/${PROJECT_NAME}
)

install(
  TARGETS asr_autopilot_core
  EXPORT export_asr_autopilot_core
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)


install(
  DIRECTORY include/
  DESTINATION include
)

ament_export_include_directories(include)

ament_export_dependencies(rclcpp asr_comms px4_msgs)
ament_export_targets(export_asr_autopilot_core HAS_LIBRARY_TARGET)
# Mark the package as complete
ament_package() 