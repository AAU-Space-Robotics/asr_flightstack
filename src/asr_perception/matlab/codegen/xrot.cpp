//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xrot.cpp
//
// Code generation for function 'xrot'
//

// Include files
#include "xrot.h"
#include "rt_nonfinite.h"

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
void b_xrot(double x_data[], int ix0, int iy0, double c, double s)
{
  double temp;
  double temp_tmp;
  temp = x_data[iy0 - 1];
  temp_tmp = x_data[ix0 - 1];
  x_data[iy0 - 1] = c * temp - s * temp_tmp;
  x_data[ix0 - 1] = c * temp_tmp + s * temp;
  temp = c * x_data[ix0] + s * x_data[iy0];
  x_data[iy0] = c * x_data[iy0] - s * x_data[ix0];
  x_data[ix0] = temp;
  temp = x_data[iy0 + 1];
  temp_tmp = x_data[ix0 + 1];
  x_data[iy0 + 1] = c * temp - s * temp_tmp;
  x_data[ix0 + 1] = c * temp_tmp + s * temp;
}

void xrot(double x[9], int ix0, int iy0, double c, double s)
{
  double temp;
  double temp_tmp;
  temp = x[iy0 - 1];
  temp_tmp = x[ix0 - 1];
  x[iy0 - 1] = c * temp - s * temp_tmp;
  x[ix0 - 1] = c * temp_tmp + s * temp;
  temp = c * x[ix0] + s * x[iy0];
  x[iy0] = c * x[iy0] - s * x[ix0];
  x[ix0] = temp;
  temp = x[iy0 + 1];
  temp_tmp = x[ix0 + 1];
  x[iy0 + 1] = c * temp - s * temp_tmp;
  x[ix0 + 1] = c * temp_tmp + s * temp;
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xrot.cpp)
