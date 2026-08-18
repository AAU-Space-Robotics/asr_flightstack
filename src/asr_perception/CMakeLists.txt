cmake_minimum_required(VERSION 3.5)
project(asr_perception)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(asr_comms REQUIRED)
find_package(message_filters REQUIRED)
find_package(Eigen3 REQUIRED)
# --- detection_filter: unified Kalman filter for probes and ArUco markers ---
add_executable(detection_filter
  src/detection_filter.cpp
)
ament_target_dependencies(detection_filter
  rclcpp
  sensor_msgs
  geometry_msgs
  std_msgs
  asr_comms
  message_filters
)
target_include_directories(detection_filter PRIVATE src)
target_include_directories(detection_filter SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIRS})

install(TARGETS detection_filter
  DESTINATION lib/${PROJECT_NAME}
)

# --- detect_probe: Python node ---
ament_python_install_package(${PROJECT_NAME})

install(PROGRAMS
  scripts/detect_probe.py
  test/probe_filter_sim.py
  test/aruco_sim.py
  test/aruco_detector_ref.py
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES
  test/probe_filter_test.launch.py
  test/aruco_detector_test.launch.py
  DESTINATION share/${PROJECT_NAME}/launch
)

install(DIRECTORY models/
  DESTINATION share/${PROJECT_NAME}/models
)

# --- aruco_detector: MATLAB Coder generated + thin ROS2 wrapper ---
# Requires running matlab/generate_code.m first to populate matlab/codegen/.
set(ARUCO_CODEGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/matlab/codegen)

if(EXISTS "${ARUCO_CODEGEN_DIR}/detect_aruco_in_camera_frame.h")
  # MATLAB installation paths
  set(MATLAB_ROOT   /usr/local/MATLAB/R2026a)
  set(MATLAB_OCV_DIR ${MATLAB_ROOT}/toolbox/vision/builtins/src/ocv)
  # MATLAB bundles OpenCV 4.7 which has cv::aruco in objdetect
  set(MATLAB_OCV_INC ${MATLAB_ROOT}/toolbox/images/opencv/opencvcg/include)
  set(MATLAB_OCV_LIB ${MATLAB_ROOT}/bin/glnxa64)

  # Coder-generated sources (all .cpp in codegen/)
  file(GLOB ARUCO_CODEGEN_SRCS "${ARUCO_CODEGEN_DIR}/*.cpp")

  # Computer Vision Toolbox sources required by readArucoMarker codegen
  set(MATLAB_ARUCO_SRCS
    ${MATLAB_OCV_DIR}/readArucoMarkerCore.cpp
    ${MATLAB_OCV_DIR}/cgCommon.cpp
    ${MATLAB_OCV_DIR}/aruco/readArucoImpl.cpp
    ${MATLAB_OCV_DIR}/aruco/arucoDictionary.cpp
  )

  add_executable(aruco_detector
    src/aruco_detector_node.cpp
    ${ARUCO_CODEGEN_SRCS}
    ${MATLAB_ARUCO_SRCS}
  )
  ament_target_dependencies(aruco_detector
    rclcpp
    sensor_msgs
    asr_comms
  )
  target_include_directories(aruco_detector PRIVATE
    ${ARUCO_CODEGEN_DIR}
  )
  target_include_directories(aruco_detector SYSTEM PRIVATE
    ${MATLAB_ROOT}/extern/include        # tmwtypes.h
    ${MATLAB_OCV_DIR}/include            # readArucoMarkerCore_api.hpp, cgCommon.hpp
    ${MATLAB_OCV_DIR}/include/aruco      # bare readArucoImpl.hpp
    ${MATLAB_OCV_INC}                    # opencv2/ headers (4.7 with aruco in objdetect)
  )
  find_package(OpenMP REQUIRED)
  target_link_libraries(aruco_detector
    ${MATLAB_OCV_LIB}/libopencv_objdetect.so.407
    ${MATLAB_OCV_LIB}/libopencv_calib3d.so.407
    ${MATLAB_OCV_LIB}/libopencv_imgproc.so.407
    ${MATLAB_OCV_LIB}/libopencv_core.so.407
    OpenMP::OpenMP_CXX
  )

  install(TARGETS aruco_detector
    DESTINATION lib/${PROJECT_NAME}
  )
else()
  message(STATUS
    "asr_perception: ArUco codegen not found — run matlab/generate_code.m to enable aruco_detector build")
endif()

ament_package()
