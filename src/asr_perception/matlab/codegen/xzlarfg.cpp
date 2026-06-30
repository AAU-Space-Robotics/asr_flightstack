//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzlarfg.cpp
//
// Code generation for function 'xzlarfg'
//

// Include files
#include "xzlarfg.h"
#include "rt_nonfinite.h"
#include "xnrm2.h"
#include <cmath>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
double xzlarfg(int n, double &alpha1, double x[3])
{
  double tau;
  tau = 0.0;
  if (n > 0) {
    double xnorm;
    xnorm = blas::xnrm2(n - 1, x);
    if (xnorm != 0.0) {
      double beta1;
      beta1 = std::abs(alpha1);
      if (beta1 < xnorm) {
        beta1 /= xnorm;
        beta1 = xnorm * std::sqrt(beta1 * beta1 + 1.0);
      } else if (beta1 > xnorm) {
        xnorm /= beta1;
        beta1 *= std::sqrt(xnorm * xnorm + 1.0);
      } else if (std::isnan(xnorm)) {
        beta1 = rtNaN;
      } else {
        beta1 *= 1.4142135623730951;
      }
      if (alpha1 >= 0.0) {
        beta1 = -beta1;
      }
      if (std::abs(beta1) < 1.0020841800044864E-292) {
        int b_vectorUB;
        int knt;
        int scalarLB;
        int vectorUB;
        knt = 0;
        scalarLB = (((n - 1) / 2) << 1) + 2;
        vectorUB = scalarLB - 2;
        do {
          knt++;
          for (int k{2}; k <= vectorUB; k += 2) {
            _mm_storeu_pd(&x[k - 1],
                          _mm_mul_pd(_mm_set1_pd(9.9792015476736E+291),
                                     _mm_loadu_pd(&x[k - 1])));
          }
          for (int k{scalarLB}; k <= n; k++) {
            x[k - 1] *= 9.9792015476736E+291;
          }
          beta1 *= 9.9792015476736E+291;
          alpha1 *= 9.9792015476736E+291;
        } while ((std::abs(beta1) < 1.0020841800044864E-292) && (knt < 20));
        xnorm = blas::xnrm2(n - 1, x);
        beta1 = std::abs(alpha1);
        if (beta1 < xnorm) {
          beta1 /= xnorm;
          beta1 = xnorm * std::sqrt(beta1 * beta1 + 1.0);
        } else if (beta1 > xnorm) {
          xnorm /= beta1;
          beta1 *= std::sqrt(xnorm * xnorm + 1.0);
        } else if (std::isnan(xnorm)) {
          beta1 = rtNaN;
        } else {
          beta1 *= 1.4142135623730951;
        }
        if (alpha1 >= 0.0) {
          beta1 = -beta1;
        }
        tau = (beta1 - alpha1) / beta1;
        xnorm = 1.0 / (alpha1 - beta1);
        b_vectorUB = scalarLB - 2;
        for (int k{2}; k <= b_vectorUB; k += 2) {
          _mm_storeu_pd(&x[k - 1], _mm_mul_pd(_mm_set1_pd(xnorm),
                                              _mm_loadu_pd(&x[k - 1])));
        }
        for (int k{scalarLB}; k <= n; k++) {
          x[k - 1] *= xnorm;
        }
        for (int k{0}; k < knt; k++) {
          beta1 *= 1.0020841800044864E-292;
        }
        alpha1 = beta1;
      } else {
        int b_vectorUB;
        int vectorUB;
        tau = (beta1 - alpha1) / beta1;
        xnorm = 1.0 / (alpha1 - beta1);
        vectorUB = (((n - 1) / 2) << 1) + 2;
        b_vectorUB = vectorUB - 2;
        for (int k{2}; k <= b_vectorUB; k += 2) {
          _mm_storeu_pd(&x[k - 1], _mm_mul_pd(_mm_set1_pd(xnorm),
                                              _mm_loadu_pd(&x[k - 1])));
        }
        for (int k{vectorUB}; k <= n; k++) {
          x[k - 1] *= xnorm;
        }
        alpha1 = beta1;
      }
    }
  }
  return tau;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzlarfg.cpp)
