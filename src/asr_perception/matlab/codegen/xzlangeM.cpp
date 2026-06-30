//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzlangeM.cpp
//
// Code generation for function 'xzlangeM'
//

// Include files
#include "xzlangeM.h"
#include "rt_nonfinite.h"
#include <cmath>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
double b_xzlangeM(const double x[9])
{
  double y;
  int k;
  boolean_T exitg1;
  y = 0.0;
  k = 0;
  exitg1 = false;
  while (!exitg1 && (k < 9)) {
    double absxk;
    absxk = std::abs(x[k]);
    if (std::isnan(absxk)) {
      y = rtNaN;
      exitg1 = true;
    } else {
      if (absxk > y) {
        y = absxk;
      }
      k++;
    }
  }
  return y;
}

double xzlangeM(const double x_data[])
{
  double y;
  int k;
  boolean_T exitg1;
  y = 0.0;
  k = 0;
  exitg1 = false;
  while (!exitg1 && (k < 9)) {
    double absxk;
    absxk = std::abs(x_data[k]);
    if (std::isnan(absxk)) {
      y = rtNaN;
      exitg1 = true;
    } else {
      if (absxk > y) {
        y = absxk;
      }
      k++;
    }
  }
  return y;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzlangeM.cpp)
