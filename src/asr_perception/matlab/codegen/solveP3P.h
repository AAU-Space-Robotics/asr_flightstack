//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// solveP3P.h
//
// Code generation for function 'solveP3P'
//

#ifndef SOLVEP3P_H
#define SOLVEP3P_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace vision {
namespace internal {
namespace calibration {
void solveP3P(const double imagePointsIn[8], const double worldPointsIn_data[],
              const double K[9], double Rs_data[], int Rs_size[3],
              double Ts_data[], int Ts_size[2]);

}
} // namespace internal
} // namespace vision
} // namespace coder

#endif
// End of code generation (solveP3P.h)
