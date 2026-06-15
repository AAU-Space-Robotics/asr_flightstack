//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// cameraIntrinsics.h
//
// Code generation for function 'cameraIntrinsics'
//

#ifndef CAMERAINTRINSICS_H
#define CAMERAINTRINSICS_H

// Include files
#include "ImageTransformer.h"
#include "cameraIntrinsicsArray.h"
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Type Definitions
namespace coder {
class cameraIntrinsics {
public:
  void init(const double varargin_1[2], const double varargin_2[2]);
  double FocalLength[2];
  double PrincipalPoint[2];
  double ImageSize[2];
  double RadialDistortion[2];
  double TangentialDistortion[2];
  double Skew;
  double K[9];

protected:
  vision::internal::calibration::ImageTransformer UndistortMap;

private:
  array<vision::internal::codegen::cameraIntrinsicsArray, 2U>
      cameraIntrinsicsArrayData;
};

} // namespace coder

#endif
// End of code generation (cameraIntrinsics.h)
