//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xswap.cpp
//
// Code generation for function 'xswap'
//

// Include files
#include "xswap.h"
#include "rt_nonfinite.h"

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
void b_xswap(double x_data[], int ix0, int iy0)
{
  double temp;
  temp = x_data[ix0 - 1];
  x_data[ix0 - 1] = x_data[iy0 - 1];
  x_data[iy0 - 1] = temp;
  temp = x_data[ix0];
  x_data[ix0] = x_data[iy0];
  x_data[iy0] = temp;
  temp = x_data[ix0 + 1];
  x_data[ix0 + 1] = x_data[iy0 + 1];
  x_data[iy0 + 1] = temp;
}

void xswap(double x[9], int ix0, int iy0)
{
  double temp;
  temp = x[ix0 - 1];
  x[ix0 - 1] = x[iy0 - 1];
  x[iy0 - 1] = temp;
  temp = x[ix0];
  x[ix0] = x[iy0];
  x[iy0] = temp;
  temp = x[ix0 + 1];
  x[ix0 + 1] = x[iy0 + 1];
  x[iy0 + 1] = temp;
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xswap.cpp)
