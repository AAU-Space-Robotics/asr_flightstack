//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzlascl.cpp
//
// Code generation for function 'xzlascl'
//

// Include files
#include "xzlascl.h"
#include "rt_nonfinite.h"
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
void b_xzlascl(double cfrom, double cto, double A[9])
{
  double cfromc;
  double ctoc;
  boolean_T notdone;
  cfromc = cfrom;
  ctoc = cto;
  notdone = true;
  while (notdone) {
    double cfrom1;
    double cto1;
    double mul;
    cfrom1 = cfromc * 2.004168360008973E-292;
    cto1 = ctoc / 4.9896007738368E+291;
    if ((cfrom1 > ctoc) && (ctoc != 0.0)) {
      mul = 2.004168360008973E-292;
      cfromc = cfrom1;
    } else if (cto1 > cfromc) {
      mul = 4.9896007738368E+291;
      ctoc = cto1;
    } else {
      mul = ctoc / cfromc;
      notdone = false;
    }
    for (int j{0}; j < 3; j++) {
      int offset;
      offset = j * 3 - 1;
      A[offset + 1] *= mul;
      A[offset + 2] *= mul;
      A[offset + 3] *= mul;
    }
  }
}

void c_xzlascl(double cfrom, double cto, double A[3])
{
  double cfromc;
  double ctoc;
  boolean_T notdone;
  cfromc = cfrom;
  ctoc = cto;
  notdone = true;
  while (notdone) {
    double cfrom1;
    double cto1;
    double mul;
    cfrom1 = cfromc * 2.004168360008973E-292;
    cto1 = ctoc / 4.9896007738368E+291;
    if ((cfrom1 > ctoc) && (ctoc != 0.0)) {
      mul = 2.004168360008973E-292;
      cfromc = cfrom1;
    } else if (cto1 > cfromc) {
      mul = 4.9896007738368E+291;
      ctoc = cto1;
    } else {
      mul = ctoc / cfromc;
      notdone = false;
    }
    _mm_storeu_pd(&A[0], _mm_mul_pd(_mm_loadu_pd(&A[0]), _mm_set1_pd(mul)));
    A[2] *= mul;
  }
}

void xzlascl(double cfrom, double cto, int m, double A_data[], int iA0)
{
  double cfromc;
  double ctoc;
  boolean_T notdone;
  cfromc = cfrom;
  ctoc = cto;
  notdone = true;
  while (notdone) {
    double cfrom1;
    double cto1;
    double mul;
    int scalarLB;
    int vectorUB;
    cfrom1 = cfromc * 2.004168360008973E-292;
    cto1 = ctoc / 4.9896007738368E+291;
    if ((cfrom1 > ctoc) && (ctoc != 0.0)) {
      mul = 2.004168360008973E-292;
      cfromc = cfrom1;
    } else if (cto1 > cfromc) {
      mul = 4.9896007738368E+291;
      ctoc = cto1;
    } else {
      mul = ctoc / cfromc;
      notdone = false;
    }
    scalarLB = m / 2 * 2;
    vectorUB = scalarLB - 2;
    for (int i{0}; i <= vectorUB; i += 2) {
      __m128d r;
      int b_i;
      b_i = (iA0 + i) - 1;
      r = _mm_loadu_pd(&A_data[b_i]);
      r = _mm_mul_pd(r, _mm_set1_pd(mul));
      _mm_storeu_pd(&A_data[b_i], r);
    }
    for (int i{scalarLB}; i < m; i++) {
      vectorUB = (iA0 + i) - 1;
      A_data[vectorUB] *= mul;
    }
  }
}

void xzlascl(double cfrom, double cto, double A_data[])
{
  double cfromc;
  double ctoc;
  boolean_T notdone;
  cfromc = cfrom;
  ctoc = cto;
  notdone = true;
  while (notdone) {
    __m128d r;
    double cfrom1;
    double cto1;
    double mul;
    cfrom1 = cfromc * 2.004168360008973E-292;
    cto1 = ctoc / 4.9896007738368E+291;
    if ((cfrom1 > ctoc) && (ctoc != 0.0)) {
      mul = 2.004168360008973E-292;
      cfromc = cfrom1;
    } else if (cto1 > cfromc) {
      mul = 4.9896007738368E+291;
      ctoc = cto1;
    } else {
      mul = ctoc / cfromc;
      notdone = false;
    }
    r = _mm_set1_pd(mul);
    _mm_storeu_pd(&A_data[0], _mm_mul_pd(_mm_loadu_pd(&A_data[0]), r));
    _mm_storeu_pd(&A_data[2], _mm_mul_pd(_mm_loadu_pd(&A_data[2]), r));
    _mm_storeu_pd(&A_data[4], _mm_mul_pd(_mm_loadu_pd(&A_data[4]), r));
    _mm_storeu_pd(&A_data[6], _mm_mul_pd(_mm_loadu_pd(&A_data[6]), r));
    A_data[8] *= mul;
  }
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzlascl.cpp)
