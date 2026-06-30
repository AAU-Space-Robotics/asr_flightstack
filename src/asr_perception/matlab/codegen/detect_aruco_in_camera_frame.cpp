//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// detect_aruco_in_camera_frame.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

// Include files
#include "detect_aruco_in_camera_frame.h"
#include "cameraIntrinsics.h"
#include "detect_aruco_in_camera_frame_data.h"
#include "detect_aruco_in_camera_frame_initialize.h"
#include "estimateWorldCameraPose.h"
#include "readArucoMarker.h"
#include "rotationMatrixToVector.h"
#include "rt_nonfinite.h"
#include "coder_array.h"
#include <cmath>
#include <cstring>

// Function Definitions
void detect_aruco_in_camera_frame(const unsigned char image_rgb[921600],
                                  double fx, double fy, double cx, double cy,
                                  double marker_size_mm, int ids[16],
                                  float tvec[48], float rvec[48],
                                  int *num_detections)
{
  coder::cameraIntrinsics camIntrinsics;
  coder::array<double, 3U> locs;
  coder::array<double, 2U> detected_ids;
  double b_cx[2];
  double b_fx[2];
  double half;
  float objectPoints[12];
  if (!isInitialized_detect_aruco_in_camera_frame) {
    detect_aruco_in_camera_frame_initialize();
  }
  //  Detect ArUco markers (DICT_4X4_50) and return camera-frame poses.
  //  Inputs:
  //    image_rgb      uint8 H×W×3  — RGB image
  //    fx, fy         double       — focal lengths (pixels)
  //    cx, cy         double       — principal point (pixels)
  //    marker_size_mm double       — physical marker side length (mm)
  //  Outputs:
  //    ids            int32  1×MAX_MARKERS  — detected marker IDs
  //    tvec           single 3×MAX_MARKERS  — translation in camera frame
  //    (metres) rvec           single 3×MAX_MARKERS  — Rodrigues rotation
  //    vector num_detections int32  scalar
  std::memset(&ids[0], 0, 16U * sizeof(int));
  std::memset(&tvec[0], 0, 48U * sizeof(float));
  std::memset(&rvec[0], 0, 48U * sizeof(float));
  *num_detections = 0;
  //  Marker corner layout in object space (mm, z=0 plane)
  half = marker_size_mm / 2.0;
  objectPoints[0] = static_cast<float>(-half);
  objectPoints[4] = static_cast<float>(half);
  objectPoints[8] = 0.0F;
  objectPoints[1] = static_cast<float>(half);
  objectPoints[5] = static_cast<float>(half);
  objectPoints[9] = 0.0F;
  objectPoints[2] = static_cast<float>(half);
  objectPoints[6] = static_cast<float>(-half);
  objectPoints[10] = 0.0F;
  objectPoints[3] = static_cast<float>(-half);
  objectPoints[7] = static_cast<float>(-half);
  objectPoints[11] = 0.0F;
  b_fx[0] = fx;
  b_fx[1] = fy;
  b_cx[0] = cx;
  b_cx[1] = cy;
  camIntrinsics.init(b_fx, b_cx);
  coder::readArucoMarker(image_rgb, detected_ids, locs);
  if (detected_ids.size(1) != 0) {
    int n;
    n = detected_ids.size(1);
    if (n > 16) {
      n = 16;
    }
    for (int i{0}; i < n; i++) {
      double worldOrientation[9];
      double worldLocation[3];
      double d;
      double d1;
      float R[9];
      float locs_data[8];
      int locs_data_tmp;
      half = std::round(detected_ids[i]);
      if (half < 2.147483648E+9) {
        if (half >= -2.147483648E+9) {
          locs_data_tmp = static_cast<int>(half);
        } else {
          locs_data_tmp = MIN_int32_T;
        }
      } else if (half >= 2.147483648E+9) {
        locs_data_tmp = MAX_int32_T;
      } else {
        locs_data_tmp = 0;
      }
      ids[i] = locs_data_tmp;
      for (int b_i{0}; b_i < 2; b_i++) {
        locs_data_tmp = 4 * b_i + 8 * i;
        locs_data[4 * b_i] = static_cast<float>(locs[locs_data_tmp]);
        locs_data[4 * b_i + 1] = static_cast<float>(locs[locs_data_tmp + 1]);
        locs_data[4 * b_i + 2] = static_cast<float>(locs[locs_data_tmp + 2]);
        locs_data[4 * b_i + 3] = static_cast<float>(locs[locs_data_tmp + 3]);
      }
      coder::estimateWorldCameraPose(locs_data, objectPoints, camIntrinsics,
                                     worldOrientation, worldLocation);
      //  camera-frame translation (mm)
      for (int b_i{0}; b_i < 3; b_i++) {
        R[3 * b_i] = static_cast<float>(worldOrientation[b_i]);
        R[3 * b_i + 1] = static_cast<float>(worldOrientation[b_i + 3]);
        R[3 * b_i + 2] = static_cast<float>(worldOrientation[b_i + 6]);
        worldLocation[b_i] = -worldLocation[b_i];
      }
      half = worldLocation[0];
      d = worldLocation[1];
      d1 = worldLocation[2];
      for (int b_i{0}; b_i < 3; b_i++) {
        tvec[b_i + 3 * i] =
            static_cast<float>((half * R[3 * b_i] + d * R[3 * b_i + 1]) +
                               d1 * R[3 * b_i + 2]) /
            1000.0F;
      }
      //  convert mm → metres
      for (int b_i{0}; b_i < 9; b_i++) {
        worldOrientation[b_i] = R[b_i];
      }
      coder::rotationMatrixToVector(worldOrientation, worldLocation);
      rvec[3 * i] = static_cast<float>(worldLocation[0]);
      rvec[3 * i + 1] = static_cast<float>(worldLocation[1]);
      rvec[3 * i + 2] = static_cast<float>(worldLocation[2]);
    }
    *num_detections = n;
  }
}

// End of code generation (detect_aruco_in_camera_frame.cpp)
