//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// detect_aruco_in_camera_frame_terminate.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame_terminate'
//

// Include files
#include "detect_aruco_in_camera_frame_terminate.h"
#include "detect_aruco_in_camera_frame_data.h"
#include "rt_nonfinite.h"
#include "omp.h"

// Function Definitions
void detect_aruco_in_camera_frame_terminate()
{
  omp_destroy_nest_lock(&detect_aruco_in_camera_frame_nestLockGlobal);
  isInitialized_detect_aruco_in_camera_frame = false;
}

// End of code generation (detect_aruco_in_camera_frame_terminate.cpp)
