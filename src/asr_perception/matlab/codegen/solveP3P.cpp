//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// solveP3P.cpp
//
// Code generation for function 'solveP3P'
//

// Include files
#include "solveP3P.h"
#include "detect_aruco_in_camera_frame_rtwutil.h"
#include "roots.h"
#include "rt_nonfinite.h"
#include "svd.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Declarations
namespace coder {
namespace vision {
namespace internal {
namespace calibration {
static double whichComponent(double a, double b, double p, double q, double r);

}
} // namespace internal
} // namespace vision
} // namespace coder

// Function Definitions
namespace coder {
namespace vision {
namespace internal {
namespace calibration {
static double whichComponent(double a, double b, double p, double q, double r)
{
  double c;
  double d;
  c = 2.0 * b;
  d = r * r;
  if ((((a * a + ((c - 2.0) - b * d) * a) - c) + b * b) + 1.0 == 0.0) {
    c = 2.0;
  } else {
    double F;
    double F_tmp;
    double b_F;
    double b_F_tmp;
    double c_F_tmp;
    double d_F_tmp;
    F_tmp = p * p;
    b_F_tmp = q * q;
    c_F_tmp = d * F_tmp;
    c = 4.0 * (p * q * r);
    d_F_tmp = 4.0 * b_F_tmp;
    F = d * b_F_tmp;
    b_F = ((((-4.0 * F_tmp + c) + c_F_tmp) + F) - rt_powd_snf(r, 3.0) * p * q) -
          d_F_tmp;
    if ((((b_F * a + c_F_tmp) - c) + d_F_tmp == 0.0) &&
        (((b_F * b + F) + 4.0 * F_tmp) - c == 0.0)) {
      c = 3.0;
    } else if (((a + b) - 1.0 == 0.0) && (r == 0.0)) {
      c = 4.0;
    } else {
      F = F_tmp + d;
      if ((F * a - d == 0.0) && (F * b - F_tmp == 0.0) && (q == 0.0)) {
        c = 5.0;
      } else {
        double e_F_tmp;
        b_F = d * d;
        e_F_tmp = F_tmp * F_tmp;
        F_tmp *= 2.0;
        c = (e_F_tmp - F_tmp * d) + b_F;
        if (((c * a - c_F_tmp) - b_F == 0.0) &&
            ((c * b - c_F_tmp) - e_F_tmp == 0.0) &&
            (F * q - 4.0 * p * r == 0.0)) {
          c = 6.0;
        } else {
          c = rt_powd_snf(p, 4.0);
          F = rt_powd_snf(q, 4.0);
          b_F = rt_powd_snf(p, 3.0);
          e_F_tmp = rt_powd_snf(q, 3.0);
          if ((((((((((((d_F_tmp + c_F_tmp) + c) - F) - b_F * r * q) +
                     p * e_F_tmp * r) -
                    4.0 * r * p * q) *
                       b +
                   2.0 * p * e_F_tmp) -
                  F_tmp * b_F_tmp) +
                 2.0 * b_F * r * q) -
                c_F_tmp * b_F_tmp) -
               c) -
                  F ==
              0.0) {
            c = 7.0;
          } else {
            boolean_T tf;
            if ((p == 0.0) && (r == 0.0)) {
              tf = true;
            } else {
              tf = false;
            }
            if (tf) {
              c = 9.0;
            } else {
              c = 1.0;
            }
          }
        }
      }
    }
  }
  return c;
}

void solveP3P(const double imagePointsIn[8], const double worldPointsIn_data[],
              const double K[9], double Rs_data[], int Rs_size[3],
              double Ts_data[], int Ts_size[2])
{
  __m128d b_r1;
  __m128d b_r2;
  __m128d b_r3;
  __m128d r;
  __m128d r10;
  __m128d r4;
  __m128d r5;
  __m128d r6;
  __m128d r7;
  __m128d r8;
  __m128d r9;
  creal_T X_data[4];
  double A[12];
  double U[12];
  double A_data[9];
  double C_data[9];
  double b_A[9];
  double normPoints1_data[9];
  double normPoints2[9];
  double y[4];
  double accumulatedData_data[3];
  double AB2;
  double a;
  double a21;
  double b;
  double b_U;
  double b_r;
  double maxval;
  double p;
  double pbr;
  double q;
  int X_size;
  int r1;
  int r2;
  int r3;
  int rtemp;
  int vectorUB;
  for (int k{0}; k < 2; k++) {
    rtemp = k << 2;
    A[rtemp] = imagePointsIn[rtemp];
    A[rtemp + 1] = imagePointsIn[rtemp + 1];
    A[rtemp + 2] = imagePointsIn[rtemp + 2];
    A[rtemp + 3] = imagePointsIn[rtemp + 3];
  }
  A[8] = 1.0;
  A[9] = 1.0;
  A[10] = 1.0;
  A[11] = 1.0;
  std::copy(&K[0], &K[9], &b_A[0]);
  r1 = 0;
  r2 = 1;
  r3 = 2;
  maxval = std::abs(K[0]);
  a21 = std::abs(K[1]);
  if (a21 > maxval) {
    maxval = a21;
    r1 = 1;
    r2 = 0;
  }
  if (std::abs(K[2]) > maxval) {
    r1 = 2;
    r2 = 1;
    r3 = 0;
  }
  b_A[r2] = K[r2] / K[r1];
  b_A[r3] /= b_A[r1];
  b_A[r2 + 3] -= b_A[r2] * b_A[r1 + 3];
  b_A[r3 + 3] -= b_A[r3] * b_A[r1 + 3];
  b_A[r2 + 6] -= b_A[r2] * b_A[r1 + 6];
  b_A[r3 + 6] -= b_A[r3] * b_A[r1 + 6];
  if (std::abs(b_A[r3 + 3]) > std::abs(b_A[r2 + 3])) {
    rtemp = r2;
    r2 = r3;
    r3 = rtemp;
  }
  b_A[r3 + 3] /= b_A[r2 + 3];
  b_A[r3 + 6] -= b_A[r3 + 3] * b_A[r2 + 6];
  r = _mm_loadu_pd(&A[0]);
  rtemp = r1 << 2;
  b_r1 = _mm_set1_pd(b_A[r1]);
  _mm_storeu_pd(&U[rtemp], _mm_div_pd(r, b_r1));
  r = _mm_loadu_pd(&U[rtemp]);
  b_r2 = _mm_loadu_pd(&A[4]);
  X_size = r2 << 2;
  b_r3 = _mm_set1_pd(b_A[r1 + 3]);
  _mm_storeu_pd(&U[X_size], _mm_sub_pd(b_r2, _mm_mul_pd(r, b_r3)));
  r = _mm_loadu_pd(&U[rtemp]);
  b_r2 = _mm_loadu_pd(&A[8]);
  vectorUB = r3 << 2;
  r4 = _mm_set1_pd(b_A[r1 + 6]);
  _mm_storeu_pd(&U[vectorUB], _mm_sub_pd(b_r2, _mm_mul_pd(r, r4)));
  b_r2 = _mm_loadu_pd(&U[X_size]);
  r5 = _mm_set1_pd(b_A[r2 + 3]);
  _mm_storeu_pd(&U[X_size], _mm_div_pd(b_r2, r5));
  r = _mm_loadu_pd(&U[X_size]);
  b_r2 = _mm_loadu_pd(&U[vectorUB]);
  r6 = _mm_set1_pd(b_A[r2 + 6]);
  _mm_storeu_pd(&U[vectorUB], _mm_sub_pd(b_r2, _mm_mul_pd(r, r6)));
  r = _mm_loadu_pd(&U[vectorUB]);
  r7 = _mm_set1_pd(b_A[r3 + 6]);
  _mm_storeu_pd(&U[vectorUB], _mm_div_pd(r, r7));
  r = _mm_loadu_pd(&U[vectorUB]);
  b_r2 = _mm_loadu_pd(&U[X_size]);
  r8 = _mm_set1_pd(b_A[r3 + 3]);
  _mm_storeu_pd(&U[X_size], _mm_sub_pd(b_r2, _mm_mul_pd(r, r8)));
  r = _mm_loadu_pd(&U[vectorUB]);
  b_r2 = _mm_loadu_pd(&U[rtemp]);
  r9 = _mm_set1_pd(b_A[r3]);
  _mm_storeu_pd(&U[rtemp], _mm_sub_pd(b_r2, _mm_mul_pd(r, r9)));
  r = _mm_loadu_pd(&U[X_size]);
  b_r2 = _mm_loadu_pd(&U[rtemp]);
  r10 = _mm_set1_pd(b_A[r2]);
  _mm_storeu_pd(&U[rtemp], _mm_sub_pd(b_r2, _mm_mul_pd(r, r10)));
  r = _mm_loadu_pd(&A[2]);
  _mm_storeu_pd(&U[rtemp + 2], _mm_div_pd(r, b_r1));
  r = _mm_loadu_pd(&U[rtemp + 2]);
  b_r2 = _mm_loadu_pd(&A[6]);
  _mm_storeu_pd(&U[X_size + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, b_r3)));
  r = _mm_loadu_pd(&U[rtemp + 2]);
  b_r2 = _mm_loadu_pd(&A[10]);
  _mm_storeu_pd(&U[vectorUB + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, r4)));
  b_r2 = _mm_loadu_pd(&U[X_size + 2]);
  _mm_storeu_pd(&U[X_size + 2], _mm_div_pd(b_r2, r5));
  r = _mm_loadu_pd(&U[X_size + 2]);
  b_r2 = _mm_loadu_pd(&U[vectorUB + 2]);
  _mm_storeu_pd(&U[vectorUB + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, r6)));
  r = _mm_loadu_pd(&U[vectorUB + 2]);
  _mm_storeu_pd(&U[vectorUB + 2], _mm_div_pd(r, r7));
  r = _mm_loadu_pd(&U[vectorUB + 2]);
  b_r2 = _mm_loadu_pd(&U[X_size + 2]);
  _mm_storeu_pd(&U[X_size + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, r8)));
  r = _mm_loadu_pd(&U[vectorUB + 2]);
  b_r2 = _mm_loadu_pd(&U[rtemp + 2]);
  _mm_storeu_pd(&U[rtemp + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, r9)));
  r = _mm_loadu_pd(&U[X_size + 2]);
  b_r2 = _mm_loadu_pd(&U[rtemp + 2]);
  _mm_storeu_pd(&U[rtemp + 2], _mm_sub_pd(b_r2, _mm_mul_pd(r, r10)));
  for (int k{0}; k <= 10; k += 2) {
    r = _mm_loadu_pd(&U[k]);
    _mm_storeu_pd(&A[k], _mm_mul_pd(r, r));
  }
  y[0] = A[0];
  y[1] = A[1];
  y[2] = A[2];
  y[3] = A[3];
  r = _mm_loadu_pd(&y[0]);
  b_r2 = _mm_loadu_pd(&A[4]);
  _mm_storeu_pd(&y[0], _mm_add_pd(r, b_r2));
  r = _mm_loadu_pd(&y[2]);
  b_r2 = _mm_loadu_pd(&A[6]);
  _mm_storeu_pd(&y[2], _mm_add_pd(r, b_r2));
  r = _mm_loadu_pd(&y[0]);
  b_r2 = _mm_loadu_pd(&A[8]);
  _mm_storeu_pd(&y[0], _mm_add_pd(r, b_r2));
  r = _mm_loadu_pd(&y[2]);
  b_r2 = _mm_loadu_pd(&A[10]);
  _mm_storeu_pd(&y[2], _mm_add_pd(r, b_r2));
  r = _mm_loadu_pd(&y[0]);
  _mm_storeu_pd(&y[0], _mm_sqrt_pd(r));
  r = _mm_loadu_pd(&y[2]);
  _mm_storeu_pd(&y[2], _mm_sqrt_pd(r));
  std::copy(&U[0], &U[12], &A[0]);
  b_U = 0.0;
  for (int k{0}; k < 3; k++) {
    rtemp = k << 2;
    r = _mm_loadu_pd(&A[rtemp]);
    b_r2 = _mm_loadu_pd(&y[0]);
    _mm_storeu_pd(&U[rtemp], _mm_div_pd(r, b_r2));
    r = _mm_loadu_pd(&A[rtemp + 2]);
    b_r2 = _mm_loadu_pd(&y[2]);
    _mm_storeu_pd(&U[rtemp + 2], _mm_div_pd(r, b_r2));
    b_U += U[rtemp] * U[rtemp + 1];
    maxval = worldPointsIn_data[4 * k] - worldPointsIn_data[4 * k + 1];
    accumulatedData_data[k] = maxval * maxval;
  }
  AB2 = (accumulatedData_data[0] + accumulatedData_data[1]) +
        accumulatedData_data[2];
  maxval = worldPointsIn_data[1] - worldPointsIn_data[2];
  a21 = worldPointsIn_data[5] - worldPointsIn_data[6];
  pbr = worldPointsIn_data[9] - worldPointsIn_data[10];
  a = ((maxval * maxval + a21 * a21) + pbr * pbr) / AB2;
  maxval = worldPointsIn_data[0] - worldPointsIn_data[2];
  a21 = worldPointsIn_data[4] - worldPointsIn_data[6];
  pbr = worldPointsIn_data[8] - worldPointsIn_data[10];
  b = ((maxval * maxval + a21 * a21) + pbr * pbr) / AB2;
  p = 2.0 * ((U[1] * U[2] + U[5] * U[6]) + U[9] * U[10]);
  q = 2.0 * ((U[0] * U[2] + U[4] * U[6]) + U[8] * U[10]);
  b_r = 2.0 * b_U;
  if (whichComponent(a, b, p, q, b_r) != 1.0) {
    Rs_size[0] = 3;
    Rs_size[1] = 3;
    Rs_size[2] = 0;
    Ts_size[0] = 0;
    Ts_size[1] = 3;
  } else {
    double coeffs[5];
    double b_coeffs_tmp;
    double c_coeffs_tmp;
    double coeffs_tmp;
    double coeffs_tmp_tmp;
    double d_coeffs_tmp;
    double e_coeffs_tmp;
    double f_coeffs_tmp;
    double g_coeffs_tmp;
    double h_coeffs_tmp;
    double i_coeffs_tmp;
    double j_coeffs_tmp;
    double k_coeffs_tmp;
    double l_coeffs_tmp;
    double m_coeffs_tmp;
    double n_coeffs_tmp;
    double o_coeffs_tmp;
    double p_coeffs_tmp;
    double q_coeffs_tmp;
    double r_coeffs_tmp;
    double s_coeffs_tmp;
    boolean_T b_b[5];
    boolean_T exitg1;
    boolean_T isodd;
    pbr = p * b * b_r;
    maxval = -2.0 * b;
    coeffs_tmp = a * a;
    coeffs_tmp_tmp = b_r * b_r;
    a21 = b * coeffs_tmp_tmp;
    b_coeffs_tmp = 2.0 * b;
    c_coeffs_tmp = b * b;
    d_coeffs_tmp = a21 * a;
    e_coeffs_tmp = 2.0 * a;
    f_coeffs_tmp = b_coeffs_tmp * a;
    coeffs[0] =
        (((((maxval + c_coeffs_tmp) + coeffs_tmp) + 1.0) - d_coeffs_tmp) +
         f_coeffs_tmp) -
        e_coeffs_tmp;
    g_coeffs_tmp = pbr * a;
    h_coeffs_tmp = 2.0 * coeffs_tmp;
    i_coeffs_tmp = 4.0 * a;
    j_coeffs_tmp = h_coeffs_tmp * q;
    k_coeffs_tmp = b_coeffs_tmp * q;
    l_coeffs_tmp = i_coeffs_tmp * q;
    m_coeffs_tmp = 2.0 * q;
    coeffs[1] =
        (((((((maxval * q * a - j_coeffs_tmp) + a21 * q * a) - m_coeffs_tmp) +
            k_coeffs_tmp) +
           l_coeffs_tmp) +
          pbr) +
         g_coeffs_tmp) -
        c_coeffs_tmp * b_r * p;
    n_coeffs_tmp = p * p;
    o_coeffs_tmp = q * q;
    maxval = b * n_coeffs_tmp;
    p_coeffs_tmp = 2.0 * o_coeffs_tmp * a;
    q_coeffs_tmp = o_coeffs_tmp * coeffs_tmp;
    r_coeffs_tmp = 2.0 * c_coeffs_tmp;
    s_coeffs_tmp = c_coeffs_tmp * n_coeffs_tmp;
    coeffs[2] =
        (((((((((((o_coeffs_tmp + c_coeffs_tmp * coeffs_tmp_tmp) - maxval) -
                 q * pbr) +
                s_coeffs_tmp) -
               d_coeffs_tmp) +
              2.0) -
             r_coeffs_tmp) -
            g_coeffs_tmp * q) +
           h_coeffs_tmp) -
          i_coeffs_tmp) -
         p_coeffs_tmp) +
        q_coeffs_tmp;
    coeffs[3] = (((((((-c_coeffs_tmp * b_r * p + g_coeffs_tmp) - j_coeffs_tmp) +
                     q * n_coeffs_tmp * b) +
                    k_coeffs_tmp * a) +
                   l_coeffs_tmp) +
                  pbr) -
                 k_coeffs_tmp) -
                m_coeffs_tmp;
    coeffs[4] =
        (((((1.0 - e_coeffs_tmp) + b_coeffs_tmp) + c_coeffs_tmp) - maxval) +
         coeffs_tmp) -
        f_coeffs_tmp;
    for (int k{0}; k < 5; k++) {
      maxval = coeffs[k];
      b_b[k] = (!std::isinf(maxval) && !std::isnan(maxval));
    }
    isodd = true;
    rtemp = 0;
    exitg1 = false;
    while (!exitg1 && (rtemp < 5)) {
      if (!b_b[rtemp]) {
        isodd = false;
        exitg1 = true;
      } else {
        rtemp++;
      }
    }
    if (!isodd) {
      Rs_size[0] = 3;
      Rs_size[1] = 3;
      Rs_size[2] = 0;
      Ts_size[0] = 0;
      Ts_size[1] = 3;
    } else {
      double b_X_data[4];
      double b1;
      int trueCount;
      X_size = roots(coeffs, X_data);
      trueCount = 0;
      for (int k{0}; k < X_size; k++) {
        maxval = std::abs(X_data[k].im);
        b_X_data[k] = maxval;
        if (maxval < 1.0E-8) {
          trueCount++;
        }
      }
      rtemp = 0;
      for (int k{0}; k < X_size; k++) {
        if (b_X_data[k] < 1.0E-8) {
          b_X_data[rtemp] = X_data[k].re;
          rtemp++;
        }
      }
      Rs_size[0] = 3;
      Rs_size[1] = 3;
      Rs_size[2] = trueCount;
      rtemp = 9 * trueCount;
      if (rtemp - 1 >= 0) {
        std::memset(&Rs_data[0], 0,
                    static_cast<unsigned int>(rtemp) * sizeof(double));
      }
      Ts_size[0] = trueCount;
      Ts_size[1] = 3;
      rtemp = trueCount * 3;
      if (rtemp - 1 >= 0) {
        std::memset(&Ts_data[0], 0,
                    static_cast<unsigned int>(rtemp) * sizeof(double));
      }
      maxval = p * q * b_r;
      maxval = (((((n_coeffs_tmp - maxval) + coeffs_tmp_tmp) * a +
                  (n_coeffs_tmp - coeffs_tmp_tmp) * b) -
                 n_coeffs_tmp) +
                maxval) -
               coeffs_tmp_tmp;
      b1 = b * (maxval * maxval);
      for (int i{0}; i < trueCount; i++) {
        double b_y_tmp;
        double c_y_tmp;
        double d;
        double d_y_tmp;
        double e_y_tmp;
        double f_y_tmp;
        double g_y_tmp;
        double h_y_tmp;
        double y_tmp;
        maxval = rt_powd_snf(b_r, 3.0);
        a21 = rt_powd_snf(p, 3.0);
        pbr = 2.0 * b_r * q;
        d_coeffs_tmp = 2.0 * p;
        f_coeffs_tmp = d_coeffs_tmp * a;
        g_coeffs_tmp = p * coeffs_tmp_tmp;
        d = b_X_data[i];
        y_tmp = d * d;
        j_coeffs_tmp = d_coeffs_tmp * q;
        k_coeffs_tmp = 2.0 * n_coeffs_tmp;
        l_coeffs_tmp = k_coeffs_tmp * a;
        b_y_tmp = d_coeffs_tmp * coeffs_tmp_tmp;
        c_y_tmp = 4.0 * p;
        d_y_tmp = rt_powd_snf(b_r, 4.0) * p;
        e_y_tmp = k_coeffs_tmp * q * b_r;
        f_y_tmp = 2.0 * a21;
        g_y_tmp = p * o_coeffs_tmp * coeffs_tmp_tmp;
        h_y_tmp = 2.0 * maxval * q;
        maxval =
            ((((((1.0 - a) - b) * y_tmp + (a - 1.0) * q * d) - a) + b) + 1.0) *
            ((((((((((((maxval *
                            (((((coeffs_tmp + c_coeffs_tmp) - e_coeffs_tmp) -
                               b_coeffs_tmp) +
                              (2.0 - coeffs_tmp_tmp) * a * b) +
                             1.0) *
                            rt_powd_snf(d, 3.0) +
                        coeffs_tmp_tmp *
                            (((((((((((((p + p * coeffs_tmp) - pbr * a * b) +
                                       pbr * b) -
                                      pbr) -
                                     f_coeffs_tmp) -
                                    d_coeffs_tmp * b) +
                                   g_coeffs_tmp * b) +
                                  4.0 * b_r * q * a) +
                                 q * maxval * a * b) -
                                pbr * coeffs_tmp) +
                               f_coeffs_tmp * b) +
                              p * c_coeffs_tmp) -
                             g_coeffs_tmp * c_coeffs_tmp) *
                            y_tmp) +
                       ((((rt_powd_snf(b_r, 5.0) * (c_coeffs_tmp - a * b) -
                           d_y_tmp * q * b) +
                          maxval * ((((((o_coeffs_tmp - i_coeffs_tmp) -
                                        p_coeffs_tmp) +
                                       q_coeffs_tmp) +
                                      h_coeffs_tmp) -
                                     r_coeffs_tmp) +
                                    2.0)) +
                         coeffs_tmp_tmp *
                             ((((c_y_tmp * q * a - j_coeffs_tmp * a * b) +
                                j_coeffs_tmp * b) -
                               j_coeffs_tmp) -
                              j_coeffs_tmp * coeffs_tmp)) +
                        b_r * (((((s_coeffs_tmp - k_coeffs_tmp * b) +
                                  l_coeffs_tmp * b) -
                                 l_coeffs_tmp) +
                                n_coeffs_tmp) +
                               n_coeffs_tmp * coeffs_tmp)) *
                           d) +
                      ((((b_y_tmp - h_y_tmp) + a21) - e_y_tmp) + g_y_tmp) *
                          coeffs_tmp) +
                     (a21 - b_y_tmp) * c_coeffs_tmp) +
                    ((((4.0 * q * maxval - c_y_tmp * coeffs_tmp_tmp) -
                       f_y_tmp) +
                      4.0 * n_coeffs_tmp * q * b_r) -
                     d_coeffs_tmp * o_coeffs_tmp * coeffs_tmp_tmp) *
                        a) +
                   (((-2.0 * q * maxval + d_y_tmp) + e_y_tmp) - f_y_tmp) * b) +
                  ((f_y_tmp + m_coeffs_tmp * maxval) - e_y_tmp) * a * b) +
                 g_y_tmp) -
                e_y_tmp) +
               b_y_tmp) +
              a21) -
             h_y_tmp) /
            b1;
        pbr = std::sqrt(AB2 /
                        ((y_tmp + maxval * maxval) - 2.0 * d * maxval * b_U));
        a21 = maxval * pbr;
        maxval = d * pbr;
        if ((pbr > 0.0) && (a21 > 0.0) && (maxval > 0.0)) {
          double accumulatedData[3];
          double v_data[3];
          int ipiv_data[3];
          for (int k{0}; k < 3; k++) {
            rtemp = k << 2;
            b_A[3 * k] = U[rtemp] * maxval;
            b_A[3 * k + 1] = U[rtemp + 1] * a21;
            b_A[3 * k + 2] = U[rtemp + 2] * pbr;
            rtemp = k * 3;
            accumulatedData_data[k] =
                (worldPointsIn_data[rtemp % 3 + 4 * (rtemp / 3)] +
                 worldPointsIn_data[(rtemp + 1) % 3 + 4 * ((rtemp + 1) / 3)]) +
                worldPointsIn_data[(rtemp + 2) % 3 + 4 * ((rtemp + 2) / 3)];
          }
          for (int k{0}; k < 3; k++) {
            maxval = accumulatedData_data[k] / 3.0;
            accumulatedData_data[k] = maxval;
            rtemp = k * 3;
            a21 = b_A[rtemp + 2];
            pbr = ((b_A[rtemp] + b_A[rtemp + 1]) + a21) / 3.0;
            accumulatedData[k] = pbr;
            _mm_storeu_pd(&normPoints1_data[3 * k],
                          _mm_sub_pd(_mm_loadu_pd(&worldPointsIn_data[4 * k]),
                                     _mm_set1_pd(maxval)));
            r = _mm_loadu_pd(&b_A[3 * k]);
            _mm_storeu_pd(&normPoints2[3 * k], _mm_sub_pd(r, _mm_set1_pd(pbr)));
            rtemp = 3 * k + 2;
            normPoints1_data[rtemp] = worldPointsIn_data[4 * k + 2] - maxval;
            normPoints2[rtemp] = a21 - pbr;
          }
          for (int k{0}; k < 3; k++) {
            maxval = normPoints2[3 * k];
            a21 = normPoints2[3 * k + 1];
            pbr = normPoints2[3 * k + 2];
            for (int ijA{0}; ijA < 3; ijA++) {
              C_data[ijA + 3 * k] = (normPoints1_data[3 * ijA] * maxval +
                                     normPoints1_data[3 * ijA + 1] * a21) +
                                    normPoints1_data[3 * ijA + 2] * pbr;
            }
          }
          isodd = true;
          for (int k{0}; k < 9; k++) {
            if (isodd) {
              maxval = C_data[k];
              if (std::isinf(maxval) || std::isnan(maxval)) {
                isodd = false;
              }
            } else {
              isodd = false;
            }
          }
          if (isodd) {
            int U_size[2];
            ::coder::internal::svd(C_data, normPoints2, U_size, y, b_A);
          } else {
            for (int k{0}; k < 9; k++) {
              normPoints2[k] = rtNaN;
              b_A[k] = rtNaN;
            }
          }
          for (int k{0}; k < 3; k++) {
            normPoints1_data[3 * k] = 0.0;
            normPoints1_data[3 * k + 1] = 0.0;
            rtemp = 3 * k + 2;
            normPoints1_data[rtemp] = 0.0;
            maxval = b_A[k];
            r = _mm_loadu_pd(&normPoints2[0]);
            b_r2 = _mm_loadu_pd(&normPoints1_data[3 * k]);
            _mm_storeu_pd(&normPoints1_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            normPoints1_data[rtemp] += normPoints2[2] * maxval;
            maxval = b_A[k + 3];
            r = _mm_loadu_pd(&normPoints2[3]);
            b_r2 = _mm_loadu_pd(&normPoints1_data[3 * k]);
            _mm_storeu_pd(&normPoints1_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            normPoints1_data[rtemp] += normPoints2[5] * maxval;
            maxval = b_A[k + 6];
            r = _mm_loadu_pd(&normPoints2[6]);
            b_r2 = _mm_loadu_pd(&normPoints1_data[3 * k]);
            _mm_storeu_pd(&normPoints1_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            normPoints1_data[rtemp] += normPoints2[8] * maxval;
          }
          ipiv_data[0] = 1;
          r1 = 1;
          for (int b_k{0}; b_k < 2; b_k++) {
            r1++;
            ipiv_data[b_k + 1] = r1;
            r2 = 1 - b_k;
            r3 = b_k << 2;
            rtemp = 4 - b_k;
            X_size = 0;
            maxval = std::abs(normPoints1_data[r3]);
            for (int k{2}; k < rtemp; k++) {
              a21 = std::abs(normPoints1_data[(r3 + k) - 1]);
              if (a21 > maxval) {
                X_size = k - 1;
                maxval = a21;
              }
            }
            if (normPoints1_data[r3 + X_size] != 0.0) {
              if (X_size != 0) {
                rtemp = b_k + X_size;
                ipiv_data[b_k] = rtemp + 1;
                maxval = normPoints1_data[b_k];
                normPoints1_data[b_k] = normPoints1_data[rtemp];
                normPoints1_data[rtemp] = maxval;
                maxval = normPoints1_data[b_k + 3];
                normPoints1_data[b_k + 3] = normPoints1_data[rtemp + 3];
                normPoints1_data[rtemp + 3] = maxval;
                maxval = normPoints1_data[b_k + 6];
                normPoints1_data[b_k + 6] = normPoints1_data[rtemp + 6];
                normPoints1_data[rtemp + 6] = maxval;
              }
              rtemp = (r3 - b_k) + 3;
              X_size = (((rtemp - r3) - 1) / 2 * 2 + r3) + 2;
              vectorUB = X_size - 2;
              for (int k{r3 + 2}; k <= vectorUB; k += 2) {
                r = _mm_loadu_pd(&normPoints1_data[k - 1]);
                r = _mm_div_pd(r, _mm_set1_pd(normPoints1_data[r3]));
                _mm_storeu_pd(&normPoints1_data[k - 1], r);
              }
              for (int k{X_size}; k <= rtemp; k++) {
                normPoints1_data[k - 1] /= normPoints1_data[r3];
              }
            }
            rtemp = r3;
            for (int k{0}; k <= r2; k++) {
              maxval = normPoints1_data[(r3 + k * 3) + 3];
              if (maxval != 0.0) {
                X_size = (rtemp - b_k) + 6;
                for (int ijA{rtemp + 5}; ijA <= X_size; ijA++) {
                  normPoints1_data[ijA - 1] +=
                      normPoints1_data[((r3 + ijA) - rtemp) - 4] * -maxval;
                }
              }
              rtemp += 3;
            }
          }
          isodd = (ipiv_data[0] > 1);
          maxval =
              normPoints1_data[0] * normPoints1_data[4] * normPoints1_data[8];
          if (ipiv_data[1] > 2) {
            isodd = !isodd;
          }
          if (isodd) {
            maxval = -maxval;
          }
          v_data[0] = 1.0;
          v_data[1] = 1.0;
          if (std::isnan(maxval)) {
            v_data[2] = rtNaN;
          } else if (maxval < 0.0) {
            v_data[2] = -1.0;
          } else {
            v_data[2] = (maxval > 0.0);
          }
          std::memset(&normPoints1_data[0], 0, 9U * sizeof(double));
          for (int k{0}; k < 3; k++) {
            normPoints1_data[k + 3 * k] = v_data[k];
            A_data[3 * k] = 0.0;
            A_data[3 * k + 1] = 0.0;
            A_data[3 * k + 2] = 0.0;
          }
          for (int k{0}; k < 3; k++) {
            r = _mm_loadu_pd(&b_A[0]);
            b_r2 = _mm_loadu_pd(&A_data[3 * k]);
            maxval = normPoints1_data[3 * k];
            _mm_storeu_pd(&A_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            rtemp = 3 * k + 2;
            A_data[rtemp] += b_A[2] * maxval;
            C_data[3 * k] = 0.0;
            r = _mm_loadu_pd(&b_A[3]);
            b_r2 = _mm_loadu_pd(&A_data[3 * k]);
            X_size = 3 * k + 1;
            maxval = normPoints1_data[X_size];
            _mm_storeu_pd(&A_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            A_data[rtemp] += b_A[5] * maxval;
            C_data[X_size] = 0.0;
            r = _mm_loadu_pd(&b_A[6]);
            b_r2 = _mm_loadu_pd(&A_data[3 * k]);
            maxval = normPoints1_data[rtemp];
            _mm_storeu_pd(&A_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            A_data[rtemp] += b_A[8] * maxval;
            C_data[rtemp] = 0.0;
          }
          for (int k{0}; k < 3; k++) {
            r = _mm_loadu_pd(&A_data[0]);
            b_r2 = _mm_loadu_pd(&C_data[3 * k]);
            maxval = normPoints2[k];
            _mm_storeu_pd(&C_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            rtemp = 3 * k + 2;
            C_data[rtemp] += A_data[2] * maxval;
            r = _mm_loadu_pd(&A_data[3]);
            b_r2 = _mm_loadu_pd(&C_data[3 * k]);
            maxval = normPoints2[k + 3];
            _mm_storeu_pd(&C_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            C_data[rtemp] += A_data[5] * maxval;
            r = _mm_loadu_pd(&A_data[6]);
            b_r2 = _mm_loadu_pd(&C_data[3 * k]);
            maxval = normPoints2[k + 6];
            _mm_storeu_pd(&C_data[3 * k],
                          _mm_add_pd(b_r2, _mm_mul_pd(r, _mm_set1_pd(maxval))));
            C_data[rtemp] += A_data[8] * maxval;
          }
          maxval = accumulatedData_data[0];
          a21 = accumulatedData_data[1];
          pbr = accumulatedData_data[2];
          for (int k{0}; k < 3; k++) {
            d_coeffs_tmp = C_data[k];
            rtemp = 3 * k + 9 * i;
            Rs_data[rtemp] = d_coeffs_tmp;
            f_coeffs_tmp = C_data[k + 3];
            Rs_data[rtemp + 1] = f_coeffs_tmp;
            g_coeffs_tmp = C_data[k + 6];
            Rs_data[rtemp + 2] = g_coeffs_tmp;
            Ts_data[i + trueCount * k] =
                accumulatedData[k] -
                ((d_coeffs_tmp * maxval + f_coeffs_tmp * a21) +
                 g_coeffs_tmp * pbr);
          }
        }
      }
    }
  }
}

} // namespace calibration
} // namespace internal
} // namespace vision
} // namespace coder

// End of code generation (solveP3P.cpp)
