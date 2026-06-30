//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// readArucoMarker.cpp
//
// Code generation for function 'readArucoMarker'
//

// Include files
#include "readArucoMarker.h"
#include "rt_nonfinite.h"
#include "coder_array.h"
#include "readArucoMarkerCore_api.hpp"
#include <algorithm>
#include <cstring>

// Function Definitions
namespace coder {
void readArucoMarker(const unsigned char b_I[921600],
                     array<double, 2U> &varargout_1,
                     array<double, 3U> &varargout_2)
{
  static const char b_markersProc[72]{"DICT_4X4_1000DICT_5X5_1000DICT_6X6_"
                                      "1000DICT_7X7_1000DICT_ARUCO_ORIGINAL"};
  static const signed char b_markersLength[5]{13, 13, 13, 13, 19};
  static unsigned char c_I[921600];
  void *detectionsObj;
  array<double, 3U> locs;
  array<double, 3U> rejectionsCell;
  array<double, 3U> rotMatrices;
  array<double, 2U> transVectors;
  array<int, 2U> famLength;
  array<char, 2U> familyNames;
  ArucoDetectorParams r;
  double camMatrix[9];
  double distCoeffs[5];
  int markersLength[5];
  int n;
  int poseLength;
  int rejectionLength;
  char markersProc[72];
  std::copy(&b_I[0], &b_I[921600], &c_I[0]);
  std::copy(&b_markersProc[0], &b_markersProc[72], &markersProc[0]);
  std::memset(&camMatrix[0], 0, 9U * sizeof(double));
  r.adaptiveThreshWinSizeMin = 3;
  r.adaptiveThreshWinSizeMax = 10;
  r.adaptiveThreshWinSizeStep = 2;
  r.adaptiveThreshConstant = 7.0;
  r.minMarkerPerimeterRate = 0.03;
  r.maxMarkerPerimeterRate = 4.0;
  r.polygonalApproxAccuracyRate = 0.03;
  r.minCornerDistanceRate = 0.05;
  r.minDistanceToBorder = 3;
  r.minMarkerDistanceRate = 0.05;
  r.perspectiveRemovePixelPerCell = 20;
  r.perspectiveRemoveIgnoredMarginPerCell = 0.13;
  r.markerBorderBits = 1;
  r.minOtsuStdDev = 5.0;
  r.maxErroneousBitsInBorderRate = 0.35;
  r.errorCorrectionRate = 0.6;
  r.detectInvertedMarker = false;
  r.useAruco3Detection = false;
  r.minSideLengthCanonicalImg = 32;
  r.minMarkerLengthRatioOriginalImg = 0.0F;
  r.cornerRefinementMethod = 1.0;
  r.cornerRefinementWinSize = 5;
  r.cornerRefinementMaxIterations = 100;
  r.cornerRefinementMinAccuracy = 0.01;
  detectionsObj = nullptr;
  n = 0;
  rejectionLength = 0;
  poseLength = 0;
  for (int i{0}; i < 5; i++) {
    distCoeffs[i] = 0.0;
    markersLength[i] = b_markersLength[i];
  }
  unsigned int numDetections;
  numDetections = readArucoImplCore(
      &c_I[0], true, 480, 640, &markersProc[0], 5, &markersLength[0], r, false,
      0.0, &camMatrix[0], &distCoeffs[0], &detectionsObj, &n, &rejectionLength,
      &poseLength);
  locs.set_size(4, 2, static_cast<int>(numDetections));
  rejectionsCell.set_size(4, 2, rejectionLength);
  varargout_1.set_size(1, static_cast<int>(numDetections));
  famLength.set_size(1, static_cast<int>(numDetections));
  familyNames.set_size(1, n);
  for (int i{0}; i < n; i++) {
    familyNames[i] = ' ';
  }
  rotMatrices.set_size(3, 3, poseLength);
  transVectors.set_size(3, poseLength);
  assignOutputs(&varargout_1[0], &locs[0], &famLength[0], &familyNames[0],
                &rejectionsCell[0], &rotMatrices[0], &transVectors[0],
                detectionsObj, false);
  deleteResultPtr(detectionsObj);
  varargout_2.set_size(4, 2, locs.size(2));
  rejectionLength = 8 * locs.size(2);
  for (int i{0}; i < rejectionLength; i++) {
    varargout_2[i] = locs[i];
  }
}

} // namespace coder

// End of code generation (readArucoMarker.cpp)
