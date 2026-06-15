//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// eml_rand_mt19937ar.cpp
//
// Code generation for function 'eml_rand_mt19937ar'
//

// Include files
#include "eml_rand_mt19937ar.h"
#include "rt_nonfinite.h"

// Function Definitions
namespace coder {
namespace internal {
namespace randfun {
void eml_rand_mt19937ar(unsigned int b_state[625])
{
  unsigned int r;
  r = 5489U;
  b_state[0] = 5489U;
  for (int mti{0}; mti < 623; mti++) {
    r = ((r ^ r >> 30U) * 1812433253U + static_cast<unsigned int>(mti)) + 1U;
    b_state[mti + 1] = r;
  }
  b_state[624] = 624U;
}

} // namespace randfun
} // namespace internal
} // namespace coder

// End of code generation (eml_rand_mt19937ar.cpp)
