//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xaxpy.cpp
//
// Code generation for function 'xaxpy'
//

// Include files
#include "xaxpy.h"
#include "rt_nonfinite.h"
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
void b_xaxpy(double a, const double x[9], int ix0, double y[3])
{
  if (!(a == 0.0)) {
    _mm_storeu_pd(&y[1], _mm_add_pd(_mm_loadu_pd(&y[1]),
                                    _mm_mul_pd(_mm_set1_pd(a),
                                               _mm_loadu_pd(&x[ix0 - 1]))));
  }
}

void b_xaxpy(double a, const double x[3], double y[9], int iy0)
{
  if (!(a == 0.0)) {
    __m128d r;
    __m128d r1;
    int i;
    i = iy0 - 1;
    r = _mm_loadu_pd(&x[1]);
    r = _mm_mul_pd(_mm_set1_pd(a), r);
    r1 = _mm_loadu_pd(&y[i]);
    r = _mm_add_pd(r1, r);
    _mm_storeu_pd(&y[i], r);
  }
}

void b_xaxpy(int n, double a, int ix0, double y[9], int iy0)
{
  if (!(a == 0.0)) {
    for (int k{0}; k < n; k++) {
      int i;
      i = (iy0 + k) - 1;
      y[i] += a * y[(ix0 + k) - 1];
    }
  }
}

void xaxpy(double a, const double x_data[], int ix0, double y_data[])
{
  if (!(a == 0.0)) {
    _mm_storeu_pd(
        &y_data[1],
        _mm_add_pd(_mm_loadu_pd(&y_data[1]),
                   _mm_mul_pd(_mm_set1_pd(a), _mm_loadu_pd(&x_data[ix0 - 1]))));
  }
}

void xaxpy(double a, const double x_data[], double y_data[], int iy0)
{
  if (!(a == 0.0)) {
    _mm_storeu_pd(
        &y_data[iy0 - 1],
        _mm_add_pd(_mm_loadu_pd(&y_data[iy0 - 1]),
                   _mm_mul_pd(_mm_set1_pd(a), _mm_loadu_pd(&x_data[1]))));
  }
}

void xaxpy(int n, double a, int ix0, double y_data[], int iy0)
{
  if (!(a == 0.0)) {
    for (int k{0}; k < n; k++) {
      int i;
      i = (iy0 + k) - 1;
      y_data[i] += a * y_data[(ix0 + k) - 1];
    }
  }
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xaxpy.cpp)
