//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xdlahqr.h
//
// Code generation for function 'xdlahqr'
//

#ifndef XDLAHQR_H
#define XDLAHQR_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
namespace reflapack {
int xdlahqr(int ilo, int ihi, double h_data[], const int h_size[2],
            double wr_data[], int &wr_size, double wi_data[], int &wi_size);

}
} // namespace internal
} // namespace coder

#endif
// End of code generation (xdlahqr.h)
