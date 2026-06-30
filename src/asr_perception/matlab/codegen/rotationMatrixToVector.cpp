//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// rotationMatrixToVector.cpp
//
// Code generation for function 'rotationMatrixToVector'
//

// Include files
#include "rotationMatrixToVector.h"
#include "rt_nonfinite.h"
#include "svd.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
void rotationMatrixToVector(const double rotationMatrix[9],
                            double rotationVector[3])
{
  __m128d r;
  double U[9];
  double V[9];
  double b_rotationMatrix[9];
  double absxk;
  double t;
  double theta;
  int rotationMatrix_tmp;
  boolean_T p;
  for (int k{0}; k < 3; k++) {
    b_rotationMatrix[3 * k] = rotationMatrix[k];
    b_rotationMatrix[3 * k + 1] = rotationMatrix[k + 3];
    b_rotationMatrix[3 * k + 2] = rotationMatrix[k + 6];
  }
  p = true;
  for (int k{0}; k < 9; k++) {
    if (p) {
      t = b_rotationMatrix[k];
      if (std::isinf(t) || std::isnan(t)) {
        p = false;
      }
    } else {
      p = false;
    }
  }
  if (p) {
    internal::svd(b_rotationMatrix, U, rotationVector, V);
  } else {
    for (int k{0}; k < 9; k++) {
      U[k] = rtNaN;
      V[k] = rtNaN;
    }
  }
  std::memset(&b_rotationMatrix[0], 0, 9U * sizeof(double));
  for (int k{0}; k < 3; k++) {
    __m128d r1;
    t = V[k];
    r = _mm_loadu_pd(&U[0]);
    r1 = _mm_loadu_pd(&b_rotationMatrix[3 * k]);
    _mm_storeu_pd(&b_rotationMatrix[3 * k],
                  _mm_add_pd(r1, _mm_mul_pd(r, _mm_set1_pd(t))));
    rotationMatrix_tmp = 3 * k + 2;
    b_rotationMatrix[rotationMatrix_tmp] += U[2] * t;
    t = V[k + 3];
    r = _mm_loadu_pd(&U[3]);
    r1 = _mm_loadu_pd(&b_rotationMatrix[3 * k]);
    _mm_storeu_pd(&b_rotationMatrix[3 * k],
                  _mm_add_pd(r1, _mm_mul_pd(r, _mm_set1_pd(t))));
    b_rotationMatrix[rotationMatrix_tmp] += U[5] * t;
    t = V[k + 6];
    r = _mm_loadu_pd(&U[6]);
    r1 = _mm_loadu_pd(&b_rotationMatrix[3 * k]);
    _mm_storeu_pd(&b_rotationMatrix[3 * k],
                  _mm_add_pd(r1, _mm_mul_pd(r, _mm_set1_pd(t))));
    b_rotationMatrix[rotationMatrix_tmp] += U[8] * t;
  }
  t = (b_rotationMatrix[0] + b_rotationMatrix[4]) + b_rotationMatrix[8];
  theta = std::acos((t - 1.0) / 2.0);
  rotationVector[0] = b_rotationMatrix[5] - b_rotationMatrix[7];
  rotationVector[1] = b_rotationMatrix[6] - b_rotationMatrix[2];
  rotationVector[2] = b_rotationMatrix[1] - b_rotationMatrix[3];
  absxk = std::sin(theta);
  if (absxk >= 0.0001) {
    t = 1.0 / (2.0 * absxk);
    r = _mm_loadu_pd(&rotationVector[0]);
    _mm_storeu_pd(
        &rotationVector[0],
        _mm_mul_pd(_mm_set1_pd(theta), _mm_mul_pd(r, _mm_set1_pd(t))));
    rotationVector[2] = theta * (rotationVector[2] * t);
  } else if (t - 1.0 > 0.0) {
    t = 0.5 - (t - 3.0) / 12.0;
    r = _mm_loadu_pd(&rotationVector[0]);
    _mm_storeu_pd(&rotationVector[0], _mm_mul_pd(_mm_set1_pd(t), r));
    rotationVector[2] *= t;
  } else {
    double b_t;
    double y;
    int idx;
    int iindx;
    rotationVector[0] = b_rotationMatrix[0];
    rotationVector[1] = b_rotationMatrix[4];
    rotationVector[2] = b_rotationMatrix[8];
    if (!std::isnan(b_rotationMatrix[0])) {
      idx = 0;
    } else {
      boolean_T exitg1;
      idx = -1;
      rotationMatrix_tmp = 2;
      exitg1 = false;
      while (!exitg1 && (rotationMatrix_tmp < 4)) {
        if (!std::isnan(rotationVector[rotationMatrix_tmp - 1])) {
          idx = rotationMatrix_tmp - 1;
          exitg1 = true;
        } else {
          rotationMatrix_tmp++;
        }
      }
    }
    if (idx + 1 == 0) {
      iindx = 0;
    } else {
      t = rotationVector[idx];
      iindx = idx;
      for (int k{idx + 2}; k < 4; k++) {
        absxk = rotationVector[k - 1];
        if (t < absxk) {
          t = absxk;
          iindx = k - 1;
        }
      }
    }
    rotationMatrix_tmp =
        static_cast<int>(std::fmod(static_cast<double>(iindx) + 1.0, 3.0));
    idx = static_cast<int>(
        std::fmod((static_cast<double>(iindx) + 1.0) + 1.0, 3.0));
    t = std::sqrt(
        ((b_rotationMatrix[iindx + 3 * iindx] -
          b_rotationMatrix[rotationMatrix_tmp + 3 * rotationMatrix_tmp]) -
         b_rotationMatrix[idx + 3 * idx]) +
        1.0);
    rotationVector[0] = 0.0;
    rotationVector[1] = 0.0;
    rotationVector[2] = 0.0;
    rotationVector[iindx] = t / 2.0;
    t *= 2.0;
    rotationVector[rotationMatrix_tmp] =
        (b_rotationMatrix[rotationMatrix_tmp + 3 * iindx] +
         b_rotationMatrix[iindx + 3 * rotationMatrix_tmp]) /
        t;
    rotationVector[idx] = (b_rotationMatrix[idx + 3 * iindx] +
                           b_rotationMatrix[iindx + 3 * idx]) /
                          t;
    t = 3.312168642111238E-170;
    absxk = std::abs(rotationVector[0]);
    if (absxk > 3.312168642111238E-170) {
      y = 1.0;
      t = absxk;
    } else {
      b_t = absxk / 3.312168642111238E-170;
      y = b_t * b_t;
    }
    absxk = std::abs(rotationVector[1]);
    if (absxk > t) {
      b_t = t / absxk;
      y = y * b_t * b_t + 1.0;
      t = absxk;
    } else {
      b_t = absxk / t;
      y += b_t * b_t;
    }
    absxk = std::abs(rotationVector[2]);
    if (absxk > t) {
      b_t = t / absxk;
      y = y * b_t * b_t + 1.0;
      t = absxk;
    } else {
      b_t = absxk / t;
      y += b_t * b_t;
    }
    y = t * std::sqrt(y);
    p = std::isnan(y);
    if (p) {
      rotationMatrix_tmp = 0;
      int exitg2;
      do {
        exitg2 = 0;
        if (rotationMatrix_tmp < 3) {
          if (std::isnan(rotationVector[rotationMatrix_tmp])) {
            exitg2 = 1;
          } else {
            rotationMatrix_tmp++;
          }
        } else {
          y = rtInf;
          exitg2 = 1;
        }
      } while (exitg2 == 0);
    }
    r = _mm_loadu_pd(&rotationVector[0]);
    _mm_storeu_pd(
        &rotationVector[0],
        _mm_div_pd(_mm_mul_pd(_mm_set1_pd(theta), r), _mm_set1_pd(y)));
    rotationVector[2] = theta * rotationVector[2] / y;
  }
}

} // namespace coder

// End of code generation (rotationMatrixToVector.cpp)
