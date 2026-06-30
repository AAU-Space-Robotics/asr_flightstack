//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xnrm2.cpp
//
// Code generation for function 'xnrm2'
//

// Include files
#include "xnrm2.h"
#include "rt_nonfinite.h"
#include <cmath>

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
double b_xnrm2(int n, const double x_data[], int ix0)
{
  double scale;
  double y;
  int kend;
  boolean_T b;
  y = 0.0;
  scale = 3.312168642111238E-170;
  kend = ix0 + n;
  for (int k{ix0}; k < kend; k++) {
    double absxk;
    absxk = std::abs(x_data[k - 1]);
    if (absxk > scale) {
      double t;
      t = scale / absxk;
      y = y * t * t + 1.0;
      scale = absxk;
    } else {
      double t;
      t = absxk / scale;
      y += t * t;
    }
  }
  y = scale * std::sqrt(y);
  b = std::isnan(y);
  if (b) {
    int b_k;
    b_k = ix0;
    int exitg1;
    do {
      exitg1 = 0;
      if (b_k <= kend - 1) {
        if (std::isnan(x_data[b_k - 1])) {
          exitg1 = 1;
        } else {
          b_k++;
        }
      } else {
        y = rtInf;
        exitg1 = 1;
      }
    } while (exitg1 == 0);
  }
  return y;
}

double c_xnrm2(int n, const double x[9], int ix0)
{
  double scale;
  double y;
  int kend;
  boolean_T b;
  y = 0.0;
  scale = 3.312168642111238E-170;
  kend = ix0 + n;
  for (int k{ix0}; k < kend; k++) {
    double absxk;
    absxk = std::abs(x[k - 1]);
    if (absxk > scale) {
      double t;
      t = scale / absxk;
      y = y * t * t + 1.0;
      scale = absxk;
    } else {
      double t;
      t = absxk / scale;
      y += t * t;
    }
  }
  y = scale * std::sqrt(y);
  b = std::isnan(y);
  if (b) {
    int b_k;
    b_k = ix0;
    int exitg1;
    do {
      exitg1 = 0;
      if (b_k <= kend - 1) {
        if (std::isnan(x[b_k - 1])) {
          exitg1 = 1;
        } else {
          b_k++;
        }
      } else {
        y = rtInf;
        exitg1 = 1;
      }
    } while (exitg1 == 0);
  }
  return y;
}

double xnrm2(int n, const double x_data[], int ix0)
{
  double y;
  y = 0.0;
  if (n >= 1) {
    if (n == 1) {
      y = std::abs(x_data[ix0 - 1]);
    } else {
      double scale;
      int kend;
      scale = 3.312168642111238E-170;
      kend = ix0 + n;
      for (int k{ix0}; k < kend; k++) {
        double absxk;
        absxk = std::abs(x_data[k - 1]);
        if (absxk > scale) {
          double t;
          t = scale / absxk;
          y = y * t * t + 1.0;
          scale = absxk;
        } else {
          double t;
          t = absxk / scale;
          y += t * t;
        }
      }
      y = scale * std::sqrt(y);
      if (std::isnan(y)) {
        int b_k;
        b_k = ix0;
        int exitg1;
        do {
          exitg1 = 0;
          if (b_k <= kend - 1) {
            if (std::isnan(x_data[b_k - 1])) {
              exitg1 = 1;
            } else {
              b_k++;
            }
          } else {
            y = rtInf;
            exitg1 = 1;
          }
        } while (exitg1 == 0);
      }
    }
  }
  return y;
}

double xnrm2(int n, const double x[3])
{
  double y;
  y = 0.0;
  if (n >= 1) {
    if (n == 1) {
      y = std::abs(x[1]);
    } else {
      double absxk;
      double scale;
      double t;
      scale = 3.312168642111238E-170;
      absxk = std::abs(x[1]);
      if (absxk > 3.312168642111238E-170) {
        y = 1.0;
        scale = absxk;
      } else {
        t = absxk / 3.312168642111238E-170;
        y = t * t;
      }
      absxk = std::abs(x[2]);
      if (absxk > scale) {
        t = scale / absxk;
        y = y * t * t + 1.0;
        scale = absxk;
      } else {
        t = absxk / scale;
        y += t * t;
      }
      y = scale * std::sqrt(y);
      if (std::isnan(y)) {
        int k;
        k = 2;
        int exitg1;
        do {
          exitg1 = 0;
          if (k < 4) {
            if (std::isnan(x[k - 1])) {
              exitg1 = 1;
            } else {
              k++;
            }
          } else {
            y = rtInf;
            exitg1 = 1;
          }
        } while (exitg1 == 0);
      }
    }
  }
  return y;
}

double xnrm2(const double x[3])
{
  double scale;
  double y;
  boolean_T b;
  y = 0.0;
  scale = 3.312168642111238E-170;
  for (int k{2}; k < 4; k++) {
    double absxk;
    absxk = std::abs(x[k - 1]);
    if (absxk > scale) {
      double t;
      t = scale / absxk;
      y = y * t * t + 1.0;
      scale = absxk;
    } else {
      double t;
      t = absxk / scale;
      y += t * t;
    }
  }
  y = scale * std::sqrt(y);
  b = std::isnan(y);
  if (b) {
    int b_k;
    b_k = 2;
    int exitg1;
    do {
      exitg1 = 0;
      if (b_k <= 3) {
        if (std::isnan(x[b_k - 1])) {
          exitg1 = 1;
        } else {
          b_k++;
        }
      } else {
        y = rtInf;
        exitg1 = 1;
      }
    } while (exitg1 == 0);
  }
  return y;
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xnrm2.cpp)
