//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_detect_aruco_in_camera_frame_api.h
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

#ifndef _CODER_DETECT_ARUCO_IN_CAMERA_FRAME_API_H
#define _CODER_DETECT_ARUCO_IN_CAMERA_FRAME_API_H

// Include files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <algorithm>
#include <cstring>

// Variable Declarations
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

// Function Declarations
void detect_aruco_in_camera_frame(const uint8_T image_rgb[921600], real_T fx,
                                  real_T fy, real_T cx, real_T cy,
                                  real_T marker_size_mm, int32_T ids[16],
                                  real32_T tvec[48], real32_T rvec[48],
                                  int32_T *num_detections);

void detect_aruco_in_camera_frame_api(const mxArray *const prhs[6],
                                      int32_T nlhs, const mxArray *plhs[4]);

void detect_aruco_in_camera_frame_atexit();

void detect_aruco_in_camera_frame_initialize();

void detect_aruco_in_camera_frame_terminate();

void detect_aruco_in_camera_frame_xil_shutdown();

void detect_aruco_in_camera_frame_xil_terminate();

#endif
// End of code generation (_coder_detect_aruco_in_camera_frame_api.h)
