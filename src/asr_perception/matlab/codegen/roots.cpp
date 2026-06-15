//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// roots.cpp
//
// Code generation for function 'roots'
//

// Include files
#include "roots.h"
#include "rt_nonfinite.h"
#include "xdlahqr.h"
#include "xzgebal.h"
#include "xzgehrd.h"
#include "xzlascl.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
int roots(const double c[5], creal_T r_data[])
{
  double A_data[16];
  double a_data[16];
  double wi_data[4];
  double wr_data[4];
  int A_size[2];
  int j;
  int k2;
  int nTrailingZeros;
  int r_size;
  int scalarLB;
  std::memset(&r_data[0], 0, 4U * sizeof(creal_T));
  r_size = 1;
  while ((r_size <= 5) && !(c[r_size - 1] != 0.0)) {
    r_size++;
  }
  k2 = 5;
  while ((k2 >= r_size) && !(c[k2 - 1] != 0.0)) {
    k2--;
  }
  nTrailingZeros = 5 - k2;
  if (r_size < k2) {
    double ctmp[5];
    int companDim;
    boolean_T exitg1;
    companDim = k2 - r_size;
    exitg1 = false;
    while (!exitg1 && (companDim > 0)) {
      boolean_T exitg2;
      j = 0;
      exitg2 = false;
      while (!exitg2 && (j + 1 <= companDim)) {
        ctmp[j] = c[r_size + j] / c[r_size - 1];
        if (std::isinf(std::abs(ctmp[j]))) {
          exitg2 = true;
        } else {
          j++;
        }
      }
      if (j + 1 > companDim) {
        exitg1 = true;
      } else {
        r_size++;
        companDim--;
      }
    }
    if (companDim < 1) {
      r_size = 5 - k2;
    } else {
      creal_T eiga_data[4];
      int loop_ub_tmp;
      loop_ub_tmp = companDim * companDim;
      std::memset(&a_data[0], 0,
                  static_cast<unsigned int>(loop_ub_tmp) * sizeof(double));
      for (int k{0}; k <= companDim - 2; k++) {
        r_size = companDim * k;
        a_data[r_size] = -ctmp[k];
        a_data[(k + r_size) + 1] = 1.0;
      }
      a_data[companDim * (companDim - 1)] = -ctmp[companDim - 1];
      if (nTrailingZeros - 1 >= 0) {
        std::memset(&r_data[0], 0,
                    static_cast<unsigned int>(nTrailingZeros) *
                        sizeof(creal_T));
      }
      if (companDim == 1) {
        for (int k{0}; k < companDim; k++) {
          eiga_data[k].re = a_data[k];
          eiga_data[k].im = 0.0;
        }
      } else {
        double absxk;
        double anrm;
        anrm = 0.0;
        r_size = 0;
        exitg1 = false;
        while (!exitg1 && (r_size <= loop_ub_tmp - 1)) {
          absxk = std::abs(a_data[r_size]);
          if (std::isnan(absxk)) {
            anrm = rtNaN;
            exitg1 = true;
          } else {
            if (absxk > anrm) {
              anrm = absxk;
            }
            r_size++;
          }
        }
        if (std::isinf(anrm) || std::isnan(anrm)) {
          for (int k{0}; k < companDim; k++) {
            eiga_data[k].re = rtNaN;
            eiga_data[k].im = 0.0;
          }
        } else {
          boolean_T guard1;
          boolean_T scalea;
          absxk = anrm;
          scalea = false;
          guard1 = false;
          if ((anrm > 0.0) && (anrm < 6.717876107567089E-139)) {
            scalea = true;
            absxk = 6.717876107567089E-139;
            guard1 = true;
          } else if (anrm > 1.488565707357403E+138) {
            scalea = true;
            absxk = 1.488565707357403E+138;
            guard1 = true;
          }
          if (guard1) {
            double cfromc;
            double ctoc;
            boolean_T notdone;
            cfromc = anrm;
            ctoc = absxk;
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
              for (int k{0}; k < companDim; k++) {
                nTrailingZeros = k * companDim - 1;
                scalarLB = companDim / 2 * 2;
                r_size = scalarLB - 2;
                for (int i{0}; i <= r_size; i += 2) {
                  __m128d r;
                  j = (nTrailingZeros + i) + 1;
                  r = _mm_loadu_pd(&a_data[j]);
                  r = _mm_mul_pd(r, _mm_set1_pd(mul));
                  _mm_storeu_pd(&a_data[j], r);
                }
                for (int i{scalarLB}; i < companDim; i++) {
                  r_size = (nTrailingZeros + i) + 1;
                  a_data[r_size] *= mul;
                }
              }
            }
          }
          A_size[0] = companDim;
          std::copy(&a_data[0], &a_data[loop_ub_tmp], &A_data[0]);
          nTrailingZeros = internal::reflapack::xzgebal(A_data, A_size, r_size,
                                                        wr_data, scalarLB);
          internal::reflapack::xzgehrd(A_data, A_size, nTrailingZeros, r_size);
          j = internal::reflapack::xdlahqr(nTrailingZeros, r_size, A_data,
                                           A_size, wr_data, scalarLB, wi_data,
                                           j);
          if (scalea) {
            r_size = companDim - j;
            internal::reflapack::xzlascl(absxk, anrm, r_size, wr_data, j + 1);
            internal::reflapack::xzlascl(absxk, anrm, r_size, wi_data, j + 1);
            if (j != 0) {
              internal::reflapack::xzlascl(absxk, anrm, nTrailingZeros - 1,
                                           wr_data, 1);
              internal::reflapack::xzlascl(absxk, anrm, nTrailingZeros - 1,
                                           wi_data, 1);
            }
          }
          if (j != 0) {
            for (int k{nTrailingZeros}; k <= j; k++) {
              wr_data[k - 1] = rtNaN;
              wi_data[k - 1] = 0.0;
            }
          }
          for (int k{0}; k < scalarLB; k++) {
            eiga_data[k].re = wr_data[k];
            eiga_data[k].im = wi_data[k];
          }
        }
      }
      for (int k{0}; k < companDim; k++) {
        r_data[(k - k2) + 5] = eiga_data[k];
      }
      r_size = (companDim - k2) + 5;
    }
  } else {
    r_size = 5 - k2;
  }
  return r_size;
}

} // namespace coder

// End of code generation (roots.cpp)
