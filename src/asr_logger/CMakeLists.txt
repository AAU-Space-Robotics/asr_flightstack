cmake_minimum_required(VERSION 3.5)
project(asr_logger)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(asr_comms REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

set(ULOG_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ulog_cpp)

# ---------------------------------------------------------------------------
# ulog_cpp — vendored ULog reader/writer (PX4/ulog_cpp, BSD-3-Clause).
# Exported so the GCS can read .ulg files without vendoring a second copy.
# ---------------------------------------------------------------------------
add_library(ulog_cpp SHARED
  ${ULOG_CPP_DIR}/messages.cpp
  ${ULOG_CPP_DIR}/writer.cpp
  ${ULOG_CPP_DIR}/simple_writer.cpp
  ${ULOG_CPP_DIR}/reader.cpp
  ${ULOG_CPP_DIR}/data_container.cpp
)

# Headers include each other flat ("messages.hpp"), so consumers need the
# directory itself on the include path, not its parent.
target_include_directories(ulog_cpp PUBLIC
  $<BUILD_INTERFACE:${ULOG_CPP_DIR}>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}/ulog_cpp>
)

# Third-party source — don't hold it to this package's warning settings.
target_compile_options(ulog_cpp PRIVATE -Wno-pedantic)

# ---------------------------------------------------------------------------

add_executable(asr_logger src/logger_node.cpp)
ament_target_dependencies(asr_logger rclcpp asr_comms std_msgs geometry_msgs)
target_include_directories(asr_logger PRIVATE include)
target_link_libraries(asr_logger ulog_cpp)

add_executable(logger_gcs src/logger_gcs_node.cpp)
ament_target_dependencies(logger_gcs rclcpp asr_comms std_msgs)
target_link_libraries(logger_gcs ulog_cpp)

# Standalone log inspector — no ROS dependency, runs against any .ulg.
add_executable(ulog_dump src/ulog_dump.cpp)
target_link_libraries(ulog_dump ulog_cpp)

install(TARGETS asr_logger logger_gcs ulog_dump
  DESTINATION lib/${PROJECT_NAME})

install(TARGETS ulog_cpp
  EXPORT asr_logger_targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY ${ULOG_CPP_DIR}/
  DESTINATION include/${PROJECT_NAME}/ulog_cpp
  FILES_MATCHING PATTERN "*.hpp"
)

ament_export_targets(asr_logger_targets HAS_LIBRARY_TARGET)
ament_export_include_directories(include/${PROJECT_NAME}/ulog_cpp)

ament_package()
