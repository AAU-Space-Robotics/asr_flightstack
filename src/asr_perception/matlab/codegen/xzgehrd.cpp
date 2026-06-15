//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzgehrd.cpp
//
// Code generation for function 'xzgehrd'
//

// Include files
#include "xzgehrd.h"
#include "rt_nonfinite.h"
#include "xnrm2.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
void xzgehrd(double a_data[], const int a_size[2], int ilo, int ihi)
{
  double work_data[4];
  double tau_data[3];
  int n;
  n = a_size[0];
  if ((ihi - ilo) + 1 > 1) {
    int im1n;
    im1n = static_cast<unsigned char>(ilo - 1);
    if (im1n - 1 >= 0) {
      std::memset(&tau_data[0], 0,
                  static_cast<unsigned int>(im1n) * sizeof(double));
    }
    if (ihi <= n - 1) {
      std::memset(&tau_data[ihi + -1], 0,
                  static_cast<unsigned int>(n - ihi) * sizeof(double));
    }
    im1n = static_cast<signed char>(a_size[0]);
    if (im1n - 1 >= 0) {
      std::memset(&work_data[0], 0,
                  static_cast<unsigned int>(im1n) * sizeof(double));
    }
    for (int i{ilo}; i < ihi; i++) {
      __m128d r;
      double alpha1;
      double beta1;
      double xnorm;
      int alpha1_tmp;
      int b_lastc;
      int b_n;
      int exitg1;
      int in;
      int jA;
      int knt;
      int lastc;
      int lastv;
      int u0;
      boolean_T exitg2;
      im1n = (i - 1) * n;
      in = i * n;
      alpha1_tmp = i + im1n;
      alpha1 = a_data[alpha1_tmp];
      u0 = i + 2;
      if (u0 > n) {
        u0 = n;
      }
      jA = u0 + im1n;
      b_n = (ihi - i) - 1;
      tau_data[i - 1] = 0.0;
      if (b_n + 1 > 0) {
        xnorm = blas::xnrm2(b_n, a_data, jA);
        if (xnorm != 0.0) {
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
            knt = 0;
            b_lastc = jA + b_n;
            do {
              knt++;
              im1n = (b_lastc - jA) / 2 * 2 + jA;
              u0 = im1n - 2;
              for (int k{jA}; k <= u0; k += 2) {
                r = _mm_loadu_pd(&a_data[k - 1]);
                r = _mm_mul_pd(_mm_set1_pd(9.9792015476736E+291), r);
                _mm_storeu_pd(&a_data[k - 1], r);
              }
              for (int k{im1n}; k < b_lastc; k++) {
                a_data[k - 1] *= 9.9792015476736E+291;
              }
              beta1 *= 9.9792015476736E+291;
              alpha1 *= 9.9792015476736E+291;
            } while ((std::abs(beta1) < 1.0020841800044864E-292) && (knt < 20));
            xnorm = blas::xnrm2(b_n, a_data, jA);
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
            tau_data[i - 1] = (beta1 - alpha1) / beta1;
            xnorm = 1.0 / (alpha1 - beta1);
            im1n = (b_lastc - jA) / 2 * 2 + jA;
            u0 = im1n - 2;
            for (int b_k{jA}; b_k <= u0; b_k += 2) {
              r = _mm_loadu_pd(&a_data[b_k - 1]);
              r = _mm_mul_pd(_mm_set1_pd(xnorm), r);
              _mm_storeu_pd(&a_data[b_k - 1], r);
            }
            for (int b_k{im1n}; b_k < b_lastc; b_k++) {
              a_data[b_k - 1] *= xnorm;
            }
            for (int k{0}; k < knt; k++) {
              beta1 *= 1.0020841800044864E-292;
            }
            alpha1 = beta1;
          } else {
            tau_data[i - 1] = (beta1 - alpha1) / beta1;
            xnorm = 1.0 / (alpha1 - beta1);
            im1n = jA + b_n;
            u0 = (im1n - jA) / 2 * 2 + jA;
            knt = u0 - 2;
            for (int k{jA}; k <= knt; k += 2) {
              r = _mm_loadu_pd(&a_data[k - 1]);
              r = _mm_mul_pd(_mm_set1_pd(xnorm), r);
              _mm_storeu_pd(&a_data[k - 1], r);
            }
            for (int k{u0}; k < im1n; k++) {
              a_data[k - 1] *= xnorm;
            }
            alpha1 = beta1;
          }
        }
      }
      a_data[alpha1_tmp] = 1.0;
      beta1 = tau_data[i - 1];
      if (beta1 != 0.0) {
        lastv = b_n;
        im1n = alpha1_tmp + b_n;
        while ((lastv + 1 > 0) && (a_data[im1n] == 0.0)) {
          lastv--;
          im1n--;
        }
        lastc = ihi;
        exitg2 = false;
        while (!exitg2 && (lastc > 0)) {
          im1n = in + lastc;
          u0 = im1n;
          do {
            exitg1 = 0;
            if (u0 <= im1n + lastv * n) {
              if (a_data[u0 - 1] != 0.0) {
                exitg1 = 1;
              } else {
                u0 += n;
              }
            } else {
              lastc--;
              exitg1 = 2;
            }
          } while (exitg1 == 0);
          if (exitg1 == 1) {
            exitg2 = true;
          }
        }
      } else {
        lastv = -1;
        lastc = 0;
      }
      if (lastv + 1 > 0) {
        if (lastc != 0) {
          std::memset(&work_data[0], 0,
                      static_cast<unsigned int>(lastc) * sizeof(double));
          im1n = alpha1_tmp;
          u0 = (in + n * lastv) + 1;
          for (int k{in + 1}; n < 0 ? k >= u0 : k <= u0; k += n) {
            knt = k + lastc;
            for (int b_k{k}; b_k < knt; b_k++) {
              b_lastc = b_k - k;
              work_data[b_lastc] += a_data[b_k - 1] * a_data[im1n];
            }
            im1n++;
          }
        }
        if (!(-beta1 == 0.0)) {
          jA = in;
          im1n = static_cast<unsigned char>(lastv + 1);
          for (int k{0}; k < im1n; k++) {
            xnorm = a_data[alpha1_tmp + k];
            if (xnorm != 0.0) {
              xnorm *= -beta1;
              u0 = lastc + jA;
              knt = ((u0 - jA) / 2 * 2 + jA) + 1;
              b_lastc = knt - 2;
              for (int b_k{jA + 1}; b_k <= b_lastc; b_k += 2) {
                __m128d r1;
                r = _mm_loadu_pd(&work_data[(b_k - jA) - 1]);
                r = _mm_mul_pd(r, _mm_set1_pd(xnorm));
                r1 = _mm_loadu_pd(&a_data[b_k - 1]);
                r = _mm_add_pd(r1, r);
                _mm_storeu_pd(&a_data[b_k - 1], r);
              }
              for (int b_k{knt}; b_k <= u0; b_k++) {
                a_data[b_k - 1] += work_data[(b_k - jA) - 1] * xnorm;
              }
            }
            jA += n;
          }
        }
      }
      jA = (i + in) + 1;
      if (beta1 != 0.0) {
        lastv = b_n + 1;
        im1n = alpha1_tmp + b_n;
        while ((lastv > 0) && (a_data[im1n] == 0.0)) {
          lastv--;
          im1n--;
        }
        b_lastc = (n - i) - 1;
        exitg2 = false;
        while (!exitg2 && (b_lastc + 1 > 0)) {
          im1n = jA + b_lastc * n;
          u0 = im1n;
          do {
            exitg1 = 0;
            if (u0 <= (im1n + lastv) - 1) {
              if (a_data[u0 - 1] != 0.0) {
                exitg1 = 1;
              } else {
                u0++;
              }
            } else {
              b_lastc--;
              exitg1 = 2;
            }
          } while (exitg1 == 0);
          if (exitg1 == 1) {
            exitg2 = true;
          }
        }
      } else {
        lastv = 0;
        b_lastc = -1;
      }
      if (lastv > 0) {
        if (b_lastc + 1 != 0) {
          if (b_lastc >= 0) {
            std::memset(&work_data[0], 0,
                        static_cast<unsigned int>(b_lastc + 1) *
                            sizeof(double));
          }
          im1n = 0;
          u0 = jA + n * b_lastc;
          for (int k{jA}; n < 0 ? k >= u0 : k <= u0; k += n) {
            xnorm = 0.0;
            knt = k + lastv;
            for (int b_k{k}; b_k < knt; b_k++) {
              xnorm += a_data[b_k - 1] * a_data[(alpha1_tmp + b_k) - k];
            }
            work_data[im1n] += xnorm;
            im1n++;
          }
        }
        if (!(-beta1 == 0.0)) {
          for (int k{0}; k <= b_lastc; k++) {
            xnorm = work_data[k];
            if (xnorm != 0.0) {
              xnorm *= -beta1;
              im1n = lastv + jA;
              for (int b_k{jA}; b_k < im1n; b_k++) {
                a_data[b_k - 1] += a_data[(alpha1_tmp + b_k) - jA] * xnorm;
              }
            }
            jA += n;
          }
        }
      }
      a_data[alpha1_tmp] = alpha1;
    }
  }
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzgehrd.cpp)
