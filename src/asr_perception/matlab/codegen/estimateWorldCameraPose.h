//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// estimateWorldCameraPose.h
//
// Code generation for function 'estimateWorldCameraPose'
//

#ifndef ESTIMATEWORLDCAMERAPOSE_H
#define ESTIMATEWORLDCAMERAPOSE_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
namespace coder {
class cameraIntrinsics;

}

// Function Declarations
namespace coder {
void estimateWorldCameraPose(const float imagePoints_data[],
                             const float worldPoints[12],
                             const cameraIntrinsics &cameraParams,
                             double orientation[9], double location[3]);

}

#endif
// End of code generation (estimateWorldCameraPose.h)
