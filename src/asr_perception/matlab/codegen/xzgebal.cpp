//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzgebal.cpp
//
// Code generation for function 'xzgebal'
//

// Include files
#include "xzgebal.h"
#include "rt_nonfinite.h"
#include "xnrm2.h"
#include <cmath>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
int xzgebal(double A_data[], const int A_size[2], int &ihi, double scale_data[],
            int &scale_size)
{
  double temp;
  int b_ix;
  int b_k;
  int exitg5;
  int ilo;
  int ix;
  int iy;
  int j;
  int l;
  int n;
  int temp_tmp;
  boolean_T notdone;
  boolean_T skipThisRow;
  n = A_size[0];
  scale_size = n;
  for (int k{0}; k < n; k++) {
    scale_data[k] = 1.0;
  }
  b_k = 0;
  l = n;
  notdone = true;
  do {
    exitg5 = 0;
    if (notdone) {
      int exitg4;
      notdone = false;
      ilo = l;
      do {
        exitg4 = 0;
        if (ilo > 0) {
          boolean_T exitg6;
          skipThisRow = false;
          ix = 0;
          exitg6 = false;
          while (!exitg6 && (ix <= l - 1)) {
            if ((ix + 1 == ilo) ||
                !(A_data[(ilo + A_size[0] * ix) - 1] != 0.0)) {
              ix++;
            } else {
              skipThisRow = true;
              exitg6 = true;
            }
          }
          if (skipThisRow) {
            ilo--;
          } else {
            scale_data[l - 1] = ilo;
            if (ilo != l) {
              ix = (ilo - 1) * n;
              iy = (l - 1) * n;
              for (int k{0}; k < l; k++) {
                temp_tmp = ix + k;
                temp = A_data[temp_tmp];
                j = iy + k;
                A_data[temp_tmp] = A_data[j];
                A_data[j] = temp;
              }
              for (int k{0}; k < n; k++) {
                ix = k * n;
                iy = (ilo + ix) - 1;
                temp = A_data[iy];
                ix = (l + ix) - 1;
                A_data[iy] = A_data[ix];
                A_data[ix] = temp;
              }
            }
            exitg4 = 1;
          }
        } else {
          exitg4 = 2;
        }
      } while (exitg4 == 0);
      if (exitg4 == 1) {
        if (l == 1) {
          ilo = 1;
          ihi = 1;
          exitg5 = 1;
        } else {
          l--;
          notdone = true;
        }
      }
    } else {
      notdone = true;
      while (notdone) {
        boolean_T exitg6;
        notdone = false;
        j = b_k;
        exitg6 = false;
        while (!exitg6 && (j + 1 <= l)) {
          boolean_T exitg7;
          skipThisRow = false;
          ix = b_k;
          exitg7 = false;
          while (!exitg7 && (ix + 1 <= l)) {
            if ((ix + 1 == j + 1) || !(A_data[ix + A_size[0] * j] != 0.0)) {
              ix++;
            } else {
              skipThisRow = true;
              exitg7 = true;
            }
          }
          if (skipThisRow) {
            j++;
          } else {
            scale_data[b_k] = j + 1;
            if (j + 1 != b_k + 1) {
              ix = j * n;
              ilo = b_k * n;
              for (int k{0}; k < l; k++) {
                iy = ix + k;
                temp = A_data[iy];
                temp_tmp = ilo + k;
                A_data[iy] = A_data[temp_tmp];
                A_data[temp_tmp] = temp;
              }
              b_ix = ilo + j;
              iy = ilo + b_k;
              temp_tmp = n - b_k;
              for (int k{0}; k < temp_tmp; k++) {
                ix = k * n;
                j = b_ix + ix;
                temp = A_data[j];
                ix += iy;
                A_data[j] = A_data[ix];
                A_data[ix] = temp;
              }
            }
            b_k++;
            notdone = true;
            exitg6 = true;
          }
        }
      }
      ilo = b_k + 1;
      ihi = l;
      skipThisRow = false;
      exitg5 = 2;
    }
  } while (exitg5 == 0);
  if (exitg5 != 1) {
    boolean_T exitg3;
    exitg3 = false;
    while (!exitg3 && !skipThisRow) {
      int exitg2;
      skipThisRow = true;
      b_ix = b_k;
      do {
        exitg2 = 0;
        if (b_ix + 1 <= l) {
          double b_s;
          double c;
          double ca;
          double r;
          double s;
          int c_tmp;
          ix = l - b_k;
          c_tmp = b_ix * n;
          c = blas::xnrm2(ix, A_data, (c_tmp + b_k) + 1);
          temp_tmp = b_k * n + b_ix;
          j = temp_tmp + 1;
          r = 0.0;
          if (ix >= 1) {
            if (ix == 1) {
              r = std::abs(A_data[temp_tmp]);
            } else {
              temp = 3.312168642111238E-170;
              iy = (temp_tmp + (ix - 1) * n) + 1;
              for (int k{j}; n < 0 ? k >= iy : k <= iy; k += n) {
                s = std::abs(A_data[k - 1]);
                if (s > temp) {
                  b_s = temp / s;
                  r = r * b_s * b_s + 1.0;
                  temp = s;
                } else {
                  b_s = s / temp;
                  r += b_s * b_s;
                }
              }
              r = temp * std::sqrt(r);
              if (std::isnan(r)) {
                ix = temp_tmp + 1;
                int exitg8;
                do {
                  exitg8 = 0;
                  if (ix <= iy) {
                    if (std::isnan(A_data[ix - 1])) {
                      exitg8 = 1;
                    } else {
                      ix += n;
                    }
                  } else {
                    r = rtInf;
                    exitg8 = 1;
                  }
                } while (exitg8 == 0);
              }
            }
          }
          ix = 1;
          if (l > 1) {
            temp = std::abs(A_data[c_tmp]);
            for (int k{2}; k <= l; k++) {
              s = std::abs(A_data[(c_tmp + k) - 1]);
              if (s > temp) {
                ix = k;
                temp = s;
              }
            }
          }
          ca = std::abs(A_data[(ix + c_tmp) - 1]);
          ix = n - b_k;
          if (ix < 1) {
            iy = 0;
          } else {
            iy = 1;
            if (ix > 1) {
              temp = std::abs(A_data[temp_tmp]);
              for (int k{2}; k <= ix; k++) {
                s = std::abs(A_data[temp_tmp + (k - 1) * n]);
                if (s > temp) {
                  iy = k;
                  temp = s;
                }
              }
            }
          }
          temp = std::abs(A_data[b_ix + A_size[0] * ((iy + b_k) - 1)]);
          if ((c == 0.0) || (r == 0.0)) {
            b_ix++;
          } else {
            double f;
            int exitg1;
            s = r / 2.0;
            f = 1.0;
            b_s = c + r;
            do {
              exitg1 = 0;
              if ((c < s) &&
                  (std::fmax(f, std::fmax(c, ca)) < 4.9896007738368E+291) &&
                  (std::fmin(r, std::fmin(s, temp)) > 2.004168360008973E-292)) {
                if (std::isnan(((((c + f) + ca) + r) + s) + temp)) {
                  exitg1 = 1;
                } else {
                  f *= 2.0;
                  c *= 2.0;
                  ca *= 2.0;
                  r /= 2.0;
                  s /= 2.0;
                  temp /= 2.0;
                }
              } else {
                s = c / 2.0;
                while ((s >= r) &&
                       (std::fmax(r, temp) < 4.9896007738368E+291) &&
                       (std::fmin(std::fmin(f, c), std::fmin(s, ca)) >
                        2.004168360008973E-292)) {
                  f /= 2.0;
                  c /= 2.0;
                  s /= 2.0;
                  ca /= 2.0;
                  r *= 2.0;
                  temp *= 2.0;
                }
                if (!(c + r >= 0.95 * b_s) &&
                    (!(f < 1.0) || !(scale_data[b_ix] < 1.0) ||
                     !(f * scale_data[b_ix] <= 1.0020841800044864E-292)) &&
                    (!(f > 1.0) || !(scale_data[b_ix] > 1.0) ||
                     !(scale_data[b_ix] >= 9.9792015476736E+291 / f))) {
                  temp = 1.0 / f;
                  scale_data[b_ix] *= f;
                  ix = (temp_tmp + n * (ix - 1)) + 1;
                  for (int k{j}; n < 0 ? k >= ix : k <= ix; k += n) {
                    A_data[k - 1] *= temp;
                  }
                  ix = c_tmp + l;
                  iy = ((ix - c_tmp) / 2 * 2 + c_tmp) + 1;
                  temp_tmp = iy - 2;
                  for (int k{c_tmp + 1}; k <= temp_tmp; k += 2) {
                    __m128d b_r;
                    b_r = _mm_loadu_pd(&A_data[k - 1]);
                    b_r = _mm_mul_pd(_mm_set1_pd(f), b_r);
                    _mm_storeu_pd(&A_data[k - 1], b_r);
                  }
                  for (int k{iy}; k <= ix; k++) {
                    A_data[k - 1] *= f;
                  }
                  skipThisRow = false;
                }
                exitg1 = 2;
              }
            } while (exitg1 == 0);
            if (exitg1 == 1) {
              exitg2 = 2;
            } else {
              b_ix++;
            }
          }
        } else {
          exitg2 = 1;
        }
      } while (exitg2 == 0);
      if (exitg2 != 1) {
        exitg3 = true;
      }
    }
  }
  return ilo;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzgebal.cpp)
