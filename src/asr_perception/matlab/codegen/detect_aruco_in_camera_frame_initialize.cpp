//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// detect_aruco_in_camera_frame_initialize.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame_initialize'
//

// Include files
#include "detect_aruco_in_camera_frame_initialize.h"
#include "detect_aruco_in_camera_frame_data.h"
#include "eml_rand_mt19937ar_stateful.h"
#include "rt_nonfinite.h"
#include "omp.h"

// Function Definitions
void detect_aruco_in_camera_frame_initialize()
{
  omp_init_nest_lock(&detect_aruco_in_camera_frame_nestLockGlobal);
  eml_rand_mt19937ar_stateful_init();
  isInitialized_detect_aruco_in_camera_frame = true;
}

// End of code generation (detect_aruco_in_camera_frame_initialize.cpp)
