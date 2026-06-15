//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// detect_aruco_in_camera_frame.h
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

#ifndef DETECT_ARUCO_IN_CAMERA_FRAME_H
#define DETECT_ARUCO_IN_CAMERA_FRAME_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void detect_aruco_in_camera_frame(const unsigned char image_rgb[921600],
                                         double fx, double fy, double cx,
                                         double cy, double marker_size_mm,
                                         int ids[16], float tvec[48],
                                         float rvec[48], int *num_detections);

#endif
// End of code generation (detect_aruco_in_camera_frame.h)
