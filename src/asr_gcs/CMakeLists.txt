cmake_minimum_required(VERSION 3.10)
project(asr_gcs)

# Use C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Fix OpenGL GLVND issue by setting the policy
if (POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

find_package(ament_cmake REQUIRED)

# asr_gcs is the ground-control-station GUI; it has no business running on the
# onboard Jetson flight computer, and the Jetson toolchain's older libstdc++
# doesn't support the C++20 chrono features the interface build relies on.
# Skip the C++ build there instead of failing.
if(EXISTS "/etc/nv_tegra_release")
  message(STATUS "asr_gcs: detected Jetson (L4T) device via /etc/nv_tegra_release - skipping interface GUI build")
  set(ASR_GCS_SKIP_INTERFACE TRUE)
else()
  set(ASR_GCS_SKIP_INTERFACE FALSE)
endif()

install(PROGRAMS
  script/GUI.py
  script/drone_state_publisher.py
  script/MotionCaptureReceiver.py
  script/probe_tester.py
  script/download_tiles.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY fonts DESTINATION share/${PROJECT_NAME}/)
install(DIRECTORY images DESTINATION share/${PROJECT_NAME}/)
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}/)
install(DIRECTORY config DESTINATION share/${PROJECT_NAME}/)

# tiles/ is a gitignored runtime cache populated by script/download_tiles.py,
# so it won't exist on a fresh clone - only install it if present.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tiles")
  install(DIRECTORY tiles DESTINATION share/${PROJECT_NAME}/)
endif()

if(NOT ASR_GCS_SKIP_INTERFACE)

# Find ROS 2 dependencies
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(asr_comms REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(asr_autopilot REQUIRED)
find_package(asr_mission REQUIRED)
find_package(px4_msgs REQUIRED)

# Fetch Dear ImGui, GLFW, ImPlot, and tinyxml2 using FetchContent
include(FetchContent)

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(imgui)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.4
)
FetchContent_MakeAvailable(glfw)

# ImPlot
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(implot)

# tinyxml2
# FetchContent_Declare(
#   tinyxml2
#   GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
#   GIT_TAG        master
# )
# Disable testing for tinyxml2
#set(BUILD_TESTING OFF CACHE BOOL "Disable tinyxml2 tests" FORCE)
#FetchContent_MakeAvailable(tinyxml2)

# Ensure tinyxml2 is built with -fPIC
#set_target_properties(tinyxml2 PROPERTIES POSITION_INDEPENDENT_CODE ON)



# Find OpenGL package
find_package(OpenGL REQUIRED)

# Include directories for external libraries
include_directories(
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${glfw_SOURCE_DIR}/include
  ${implot_SOURCE_DIR}      # Add ImPlot include directory
  #${tinyxml2_SOURCE_DIR}    # Add tinyxml2 include directory
  ${CMAKE_SOURCE_DIR}/include
)

# Interface node executable
add_executable(interface
  src/main.cpp
  src/interfaceutils.cpp
  src/app_window.cpp
  src/info_panels.cpp
  src/map.cpp
  src/planner/map_overlay.cpp
  src/widgets.cpp
  src/planner/planner.cpp
  src/planner/planner_panel.cpp
  src/planner/save_load_dialog.cpp
  src/planner/height_chart.cpp
  src/planner/mission_control_bar.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${implot_SOURCE_DIR}/implot.cpp           # Add ImPlot source
  ${implot_SOURCE_DIR}/implot_items.cpp     # Add ImPlot items source
)

ament_target_dependencies(interface
  rclcpp
  rclcpp_action
  std_msgs
  asr_comms
  ament_index_cpp
  asr_autopilot
  px4_msgs
)

install(TARGETS interface
  DESTINATION lib/${PROJECT_NAME}
)
target_link_libraries(interface
  OpenGL::GL      # Link OpenGL
  glfw            # Link GLFW
  tinyxml2        # Link tinyxml2 from FetchContent
  ${OpenCV_LIBS}
  asr_autopilot::asr_autopilot_core
  asr_mission::asr_mission_lib
)
target_include_directories(interface
  PRIVATE ${OpenCV_INCLUDE_DIRS}
)

endif() # NOT ASR_GCS_SKIP_INTERFACE

# Ament package
ament_package()
