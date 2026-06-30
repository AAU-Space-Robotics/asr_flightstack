//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// cameraIntrinsics.cpp
//
// Code generation for function 'cameraIntrinsics'
//

// Include files
#include "cameraIntrinsics.h"
#include "cameraIntrinsicsArray.h"
#include "rt_nonfinite.h"
#include "coder_array.h"
#include <emmintrin.h>

// Function Definitions
namespace coder {
void cameraIntrinsics::init(const double varargin_1[2],
                            const double varargin_2[2])
{
  __m128d r;
  vision::internal::codegen::cameraIntrinsicsArray r1;
  double dv[2];
  _mm_storeu_pd(&FocalLength[0], _mm_loadu_pd(&varargin_1[0]));
  _mm_storeu_pd(&PrincipalPoint[0], _mm_loadu_pd(&varargin_2[0]));
  dv[0] = 0.0;
  dv[1] = 1.0;
  r = _mm_loadu_pd(&dv[0]);
  _mm_storeu_pd(&ImageSize[0], _mm_add_pd(_mm_mul_pd(_mm_set1_pd(160.0), r),
                                          _mm_set1_pd(480.0)));
  r = _mm_set1_pd(0.0);
  _mm_storeu_pd(&RadialDistortion[0], r);
  _mm_storeu_pd(&TangentialDistortion[0], r);
  Skew = 0.0;
  K[0] = FocalLength[0];
  K[3] = Skew;
  K[6] = PrincipalPoint[0];
  K[1] = 0.0;
  K[4] = FocalLength[1];
  K[7] = PrincipalPoint[1];
  K[2] = 0.0;
  K[5] = 0.0;
  K[8] = 1.0;
  cameraIntrinsicsArrayData.set_size(1, 1);
  cameraIntrinsicsArrayData[0] = r1;
}

} // namespace coder

// End of code generation (cameraIntrinsics.cpp)
