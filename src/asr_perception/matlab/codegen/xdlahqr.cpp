//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xdlahqr.cpp
//
// Code generation for function 'xdlahqr'
//

// Include files
#include "xdlahqr.h"
#include "rt_nonfinite.h"
#include "xdlanv2.h"
#include "xzlarfg.h"
#include <cmath>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
int xdlahqr(int ilo, int ihi, double h_data[], const int h_size[2],
            double wr_data[], int &wr_size, double wi_data[], int &wi_size)
{
  double v[3];
  double h11;
  double h12;
  double h21;
  double h21s;
  double h22;
  double rt2r;
  double t3;
  int info;
  int n;
  int s_tmp_tmp_tmp;
  n = h_size[0];
  wr_size = n;
  wi_size = n;
  info = 0;
  s_tmp_tmp_tmp = static_cast<unsigned char>(ilo - 1);
  for (int i{0}; i < s_tmp_tmp_tmp; i++) {
    wr_data[i] = h_data[i + h_size[0] * i];
    wi_data[i] = 0.0;
  }
  for (int i{ihi + 1}; i <= n; i++) {
    wr_data[i - 1] = h_data[(i + h_size[0] * (i - 1)) - 1];
    wi_data[i - 1] = 0.0;
  }
  if (ilo == ihi) {
    wr_data[ilo - 1] = h_data[(ilo + h_size[0] * (ilo - 1)) - 1];
    wi_data[ilo - 1] = 0.0;
  } else {
    double smlnum;
    int b_i;
    int kdefl;
    boolean_T exitg1;
    for (int i{ilo}; i <= ihi - 3; i++) {
      h_data[2] = 0.0;
      h_data[3] = 0.0;
    }
    if (ilo <= ihi - 2) {
      h_data[(ihi + h_size[0] * (ihi - 3)) - 1] = 0.0;
    }
    smlnum = 2.2250738585072014E-308 *
             (static_cast<double>((ihi - ilo) + 1) / 2.220446049250313E-16);
    kdefl = 0;
    b_i = ihi - 1;
    exitg1 = false;
    while (!exitg1 && (b_i + 1 >= ilo)) {
      int its;
      int l;
      int nr;
      boolean_T converged;
      boolean_T exitg2;
      l = ilo;
      converged = false;
      its = 0;
      exitg2 = false;
      while (!exitg2 && (its < 301)) {
        double s;
        int k;
        boolean_T exitg3;
        k = b_i;
        exitg3 = false;
        while (!exitg3 && (k + 1 > l)) {
          s_tmp_tmp_tmp = k + h_size[0] * (k - 1);
          rt2r = std::abs(h_data[s_tmp_tmp_tmp]);
          if (rt2r <= smlnum) {
            exitg3 = true;
          } else {
            nr = k + h_size[0] * k;
            h12 = std::abs(h_data[nr]);
            h11 = std::abs(h_data[s_tmp_tmp_tmp - 1]) + h12;
            if (h11 == 0.0) {
              if (k - 1 >= ilo) {
                h11 = std::abs(h_data[(k + h_size[0] * (k - 2)) - 1]);
              }
              if (k + 2 <= ihi) {
                h11 += std::abs(h_data[nr + 1]);
              }
            }
            if (rt2r <= 2.220446049250313E-16 * h11) {
              h21 = std::abs(h_data[nr - 1]);
              h11 = std::abs(h_data[s_tmp_tmp_tmp - 1] - h_data[nr]);
              h22 = std::fmax(h12, h11);
              h11 = std::fmin(h12, h11);
              s = h22 + h11;
              if (std::fmin(rt2r, h21) * (std::fmax(rt2r, h21) / s) <=
                  std::fmax(smlnum,
                            2.220446049250313E-16 * (h11 * (h22 / s)))) {
                exitg3 = true;
              } else {
                k--;
              }
            } else {
              k--;
            }
          }
        }
        l = k + 1;
        if (k + 1 > ilo) {
          h_data[k + h_size[0] * (k - 1)] = 0.0;
        }
        if (k + 1 >= b_i) {
          converged = true;
          exitg2 = true;
        } else {
          __m128d r;
          double rt1r;
          int m;
          kdefl++;
          if (kdefl - kdefl / 20 * 20 == 0) {
            s = std::abs(h_data[b_i + h_size[0] * (b_i - 1)]) +
                std::abs(h_data[(b_i + h_size[0] * (b_i - 2)) - 1]);
            h11 = 0.75 * s + h_data[b_i + h_size[0] * b_i];
            h12 = -0.4375 * s;
            h21 = s;
            h22 = h11;
          } else if (kdefl - kdefl / 10 * 10 == 0) {
            s_tmp_tmp_tmp = k + h_size[0] * k;
            s = std::abs(h_data[s_tmp_tmp_tmp + 1]) +
                std::abs(h_data[(k + h_size[0] * (k + 1)) + 2]);
            h11 = 0.75 * s + h_data[s_tmp_tmp_tmp];
            h12 = -0.4375 * s;
            h21 = s;
            h22 = h11;
          } else {
            nr = b_i + h_size[0] * (b_i - 1);
            h11 = h_data[nr - 1];
            h21 = h_data[nr];
            s_tmp_tmp_tmp = b_i + h_size[0] * b_i;
            h12 = h_data[s_tmp_tmp_tmp - 1];
            h22 = h_data[s_tmp_tmp_tmp];
          }
          s = ((std::abs(h11) + std::abs(h12)) + std::abs(h21)) + std::abs(h22);
          if (s == 0.0) {
            rt1r = 0.0;
            h11 = 0.0;
            rt2r = 0.0;
            h12 = 0.0;
          } else {
            h11 /= s;
            h21 /= s;
            h12 /= s;
            h22 /= s;
            t3 = (h11 + h22) / 2.0;
            h11 = (h11 - t3) * (h22 - t3) - h12 * h21;
            h12 = std::sqrt(std::abs(h11));
            if (h11 >= 0.0) {
              rt1r = t3 * s;
              rt2r = rt1r;
              h11 = h12 * s;
              h12 = -h11;
            } else {
              rt1r = t3 + h12;
              rt2r = t3 - h12;
              if (std::abs(rt1r - h22) <= std::abs(rt2r - h22)) {
                rt1r *= s;
                rt2r = rt1r;
              } else {
                rt2r *= s;
                rt1r = rt2r;
              }
              h11 = 0.0;
              h12 = 0.0;
            }
          }
          m = b_i - 1;
          exitg3 = false;
          while (!exitg3 && (m >= k + 1)) {
            s_tmp_tmp_tmp = m + h_size[0] * (m - 1);
            h21 = h_data[s_tmp_tmp_tmp - 1];
            t3 = h21 - rt2r;
            s = (std::abs(t3) + std::abs(h12)) +
                std::abs(h_data[s_tmp_tmp_tmp]);
            h21s = h_data[s_tmp_tmp_tmp] / s;
            nr = m + h_size[0] * m;
            v[0] = (h21s * h_data[nr - 1] + t3 * (t3 / s)) - h11 * (h12 / s);
            v[1] = h21s * (((h21 + h_data[nr]) - rt1r) - rt2r);
            v[2] = h21s * h_data[nr + 1];
            s = (std::abs(v[0]) + std::abs(v[1])) + std::abs(v[2]);
            r = _mm_loadu_pd(&v[0]);
            _mm_storeu_pd(&v[0], _mm_div_pd(r, _mm_set1_pd(s)));
            v[2] /= s;
            if ((m == k + 1) ||
                (std::abs(h_data[m - 1]) * (std::abs(v[1]) + std::abs(v[2])) <=
                 2.220446049250313E-16 * std::abs(v[0]) *
                     ((std::abs(h_data[0]) +
                       std::abs(h_data[s_tmp_tmp_tmp - 1])) +
                      std::abs(h_data[nr])))) {
              exitg3 = true;
            } else {
              m--;
            }
          }
          for (int c_i{m}; c_i <= b_i; c_i++) {
            nr = (b_i - c_i) + 2;
            if (nr >= 3) {
              nr = 3;
            }
            if (c_i > m) {
              s_tmp_tmp_tmp = ((c_i - 2) * n + c_i) - 1;
              for (int i{0}; i < nr; i++) {
                v[i] = h_data[s_tmp_tmp_tmp + i];
              }
            }
            h11 = v[0];
            s = xzlarfg(nr, h11, v);
            if (c_i > m) {
              s_tmp_tmp_tmp = c_i + h_size[0] * (c_i - 2);
              h_data[s_tmp_tmp_tmp - 1] = h11;
              h_data[s_tmp_tmp_tmp] = 0.0;
              if (c_i < b_i) {
                h_data[c_i + 1] = 0.0;
              }
            } else if (m > k + 1) {
              h_data[c_i - 1] *= 1.0 - s;
            }
            h22 = v[1];
            rt2r = s * v[1];
            if (nr == 3) {
              int b_scalarLB;
              int d_i;
              int i1;
              h21s = v[2];
              t3 = s * v[2];
              for (int i{c_i}; i <= b_i + 1; i++) {
                s_tmp_tmp_tmp = c_i + h_size[0] * (i - 1);
                h11 = h_data[s_tmp_tmp_tmp - 1];
                h12 = h_data[s_tmp_tmp_tmp];
                h21 = h_data[s_tmp_tmp_tmp + 1];
                rt1r = (h11 + h22 * h12) + h21s * h21;
                h11 -= rt1r * s;
                h_data[s_tmp_tmp_tmp - 1] = h11;
                h12 -= rt1r * rt2r;
                h_data[s_tmp_tmp_tmp] = h12;
                h21 -= rt1r * t3;
                h_data[s_tmp_tmp_tmp + 1] = h21;
              }
              if (c_i + 3 <= b_i + 1) {
                d_i = c_i;
              } else {
                d_i = b_i - 2;
              }
              b_scalarLB = (((((d_i - k) + 3) / 2) << 1) + k) + 1;
              s_tmp_tmp_tmp = b_scalarLB - 2;
              for (int i{k + 1}; i <= s_tmp_tmp_tmp; i += 2) {
                __m128d r1;
                __m128d r2;
                __m128d r3;
                int scalarLB;
                nr = (i + h_size[0] * c_i) - 1;
                r = _mm_loadu_pd(&h_data[nr]);
                i1 = (i + h_size[0] * (c_i + 1)) - 1;
                r1 = _mm_loadu_pd(&h_data[i1]);
                scalarLB = (i + h_size[0] * (c_i - 1)) - 1;
                r2 = _mm_loadu_pd(&h_data[scalarLB]);
                r3 = _mm_add_pd(_mm_add_pd(r2, _mm_mul_pd(_mm_set1_pd(h22), r)),
                                _mm_mul_pd(_mm_set1_pd(h21s), r1));
                _mm_storeu_pd(&h_data[scalarLB],
                              _mm_sub_pd(r2, _mm_mul_pd(r3, _mm_set1_pd(s))));
                _mm_storeu_pd(&h_data[nr],
                              _mm_sub_pd(r, _mm_mul_pd(r3, _mm_set1_pd(rt2r))));
                _mm_storeu_pd(&h_data[i1],
                              _mm_sub_pd(r1, _mm_mul_pd(r3, _mm_set1_pd(t3))));
              }
              for (int i{b_scalarLB}; i <= d_i + 3; i++) {
                s_tmp_tmp_tmp = (i + h_size[0] * (c_i - 1)) - 1;
                h11 = h_data[s_tmp_tmp_tmp];
                nr = (i + h_size[0] * c_i) - 1;
                h12 = h_data[nr];
                i1 = (i + h_size[0] * (c_i + 1)) - 1;
                h21 = h_data[i1];
                rt1r = (h11 + h22 * h12) + h21s * h21;
                h11 -= rt1r * s;
                h_data[s_tmp_tmp_tmp] = h11;
                h12 -= rt1r * rt2r;
                h_data[nr] = h12;
                h21 -= rt1r * t3;
                h_data[i1] = h21;
              }
            } else if (nr == 2) {
              int scalarLB;
              for (int i{c_i}; i <= b_i + 1; i++) {
                s_tmp_tmp_tmp = c_i + h_size[0] * (i - 1);
                h11 = h_data[s_tmp_tmp_tmp - 1];
                h12 = h_data[s_tmp_tmp_tmp];
                rt1r = h11 + h22 * h12;
                h11 -= rt1r * s;
                h_data[s_tmp_tmp_tmp - 1] = h11;
                h12 -= rt1r * rt2r;
                h_data[s_tmp_tmp_tmp] = h12;
              }
              scalarLB = (((((b_i - k) + 1) / 2) << 1) + k) + 1;
              s_tmp_tmp_tmp = scalarLB - 2;
              for (int i{k + 1}; i <= s_tmp_tmp_tmp; i += 2) {
                __m128d r1;
                __m128d r2;
                int i1;
                nr = (i + h_size[0] * c_i) - 1;
                r = _mm_loadu_pd(&h_data[nr]);
                i1 = (i + h_size[0] * (c_i - 1)) - 1;
                r1 = _mm_loadu_pd(&h_data[i1]);
                r2 = _mm_add_pd(r1, _mm_mul_pd(_mm_set1_pd(h22), r));
                _mm_storeu_pd(&h_data[i1],
                              _mm_sub_pd(r1, _mm_mul_pd(r2, _mm_set1_pd(s))));
                _mm_storeu_pd(&h_data[nr],
                              _mm_sub_pd(r, _mm_mul_pd(r2, _mm_set1_pd(rt2r))));
              }
              for (int i{scalarLB}; i <= b_i + 1; i++) {
                s_tmp_tmp_tmp = (i + h_size[0] * (c_i - 1)) - 1;
                h11 = h_data[s_tmp_tmp_tmp];
                nr = (i + h_size[0] * c_i) - 1;
                h12 = h_data[nr];
                rt1r = h11 + h22 * h12;
                h11 -= rt1r * s;
                h_data[s_tmp_tmp_tmp] = h11;
                h12 -= rt1r * rt2r;
                h_data[nr] = h12;
              }
            }
          }
          its++;
        }
      }
      if (!converged) {
        info = b_i + 1;
        exitg1 = true;
      } else {
        if (l == b_i + 1) {
          wr_data[b_i] = h_data[b_i + h_size[0] * b_i];
          wi_data[b_i] = 0.0;
        } else if (l == b_i) {
          s_tmp_tmp_tmp = b_i + h_size[0] * b_i;
          h11 = h_data[s_tmp_tmp_tmp - 1];
          nr = b_i + h_size[0] * (b_i - 1);
          h12 = h_data[nr];
          h21 = h_data[s_tmp_tmp_tmp];
          wr_data[b_i - 1] = xdlanv2(h_data[nr - 1], h11, h12, h21,
                                     wi_data[b_i - 1], h22, rt2r, t3, h21s);
          wr_data[b_i] = h22;
          wi_data[b_i] = rt2r;
          h_data[s_tmp_tmp_tmp - 1] = h11;
          h_data[nr] = h12;
          h_data[s_tmp_tmp_tmp] = h21;
        }
        kdefl = 0;
        b_i = l - 2;
      }
    }
    if ((info != 0) && (h_size[0] > 2)) {
      for (int i{3}; i <= n; i++) {
        for (int c_i{i}; c_i <= n; c_i++) {
          h_data[(c_i + h_size[0] * (i - 3)) - 1] = 0.0;
        }
      }
    }
  }
  return info;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xdlahqr.cpp)
