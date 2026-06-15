//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// svd.h
//
// Code generation for function 'svd'
//

#ifndef SVD_H
#define SVD_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
int svd(const double A_data[], double U_data[], int U_size[2], double s_data[],
        double V[9]);

void svd(const double A[9], double U[9], double s[3], double V[9]);

} // namespace internal
} // namespace coder

#endif
// End of code generation (svd.h)
