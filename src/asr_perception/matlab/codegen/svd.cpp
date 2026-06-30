//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// svd.cpp
//
// Code generation for function 'svd'
//

// Include files
#include "svd.h"
#include "rt_nonfinite.h"
#include "xaxpy.h"
#include "xdotc.h"
#include "xnrm2.h"
#include "xrot.h"
#include "xrotg.h"
#include "xswap.h"
#include "xzlangeM.h"
#include "xzlascl.h"
#include <algorithm>
#include <cmath>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
int svd(const double A_data[], double U_data[], int U_size[2], double s_data[],
        double V[9])
{
  __m128d r;
  double x_data[16];
  double b_A_data[9];
  double b_s_data[3];
  double e[3];
  double work_data[3];
  double anrm;
  double b;
  double cscale;
  double f;
  double nrm;
  double rt;
  double sm;
  double snorm;
  double sqds;
  int iter;
  int m;
  int qp1;
  int qq;
  int qs;
  int s_size;
  int vectorUB;
  boolean_T doscale;
  b_s_data[0] = 0.0;
  e[0] = 0.0;
  work_data[0] = 0.0;
  b_s_data[1] = 0.0;
  e[1] = 0.0;
  work_data[1] = 0.0;
  b_s_data[2] = 0.0;
  e[2] = 0.0;
  work_data[2] = 0.0;
  U_size[0] = 3;
  U_size[1] = 3;
  for (int k{0}; k < 9; k++) {
    b_A_data[k] = A_data[k];
    U_data[k] = 0.0;
    V[k] = 0.0;
  }
  doscale = false;
  anrm = reflapack::xzlangeM(A_data);
  cscale = anrm;
  if ((anrm > 0.0) && (anrm < 6.717876107567089E-139)) {
    doscale = true;
    cscale = 6.717876107567089E-139;
    reflapack::xzlascl(anrm, cscale, b_A_data);
  } else if (anrm > 1.488565707357403E+138) {
    doscale = true;
    cscale = 1.488565707357403E+138;
    reflapack::xzlascl(anrm, cscale, b_A_data);
  }
  for (int q{0}; q < 2; q++) {
    boolean_T apply_transform;
    qp1 = q + 2;
    iter = q + 3 * q;
    qq = iter + 1;
    apply_transform = false;
    nrm = blas::b_xnrm2(3 - q, b_A_data, iter + 1);
    if (nrm > 0.0) {
      apply_transform = true;
      if (b_A_data[iter] < 0.0) {
        nrm = -nrm;
      }
      b_s_data[q] = nrm;
      if (std::abs(nrm) >= 1.0020841800044864E-292) {
        nrm = 1.0 / nrm;
        s_size = (iter - q) + 3;
        qs = ((((s_size - iter) / 2) << 1) + iter) + 1;
        vectorUB = qs - 2;
        for (int k{qq}; k <= vectorUB; k += 2) {
          r = _mm_loadu_pd(&b_A_data[k - 1]);
          _mm_storeu_pd(&b_A_data[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
        }
        for (int k{qs}; k <= s_size; k++) {
          b_A_data[k - 1] *= nrm;
        }
      } else {
        s_size = (iter - q) + 3;
        qs = ((((s_size - iter) / 2) << 1) + iter) + 1;
        vectorUB = qs - 2;
        for (int k{qq}; k <= vectorUB; k += 2) {
          r = _mm_loadu_pd(&b_A_data[k - 1]);
          _mm_storeu_pd(&b_A_data[k - 1],
                        _mm_div_pd(r, _mm_set1_pd(b_s_data[q])));
        }
        for (int k{qs}; k <= s_size; k++) {
          b_A_data[k - 1] /= b_s_data[q];
        }
      }
      b_A_data[iter]++;
      b_s_data[q] = -b_s_data[q];
    } else {
      b_s_data[q] = 0.0;
    }
    for (int k{qp1}; k < 4; k++) {
      s_size = q + 3 * (k - 1);
      if (apply_transform) {
        blas::xaxpy(
            3 - q,
            -(blas::xdotc(3 - q, b_A_data, iter + 1, b_A_data, s_size + 1) /
              b_A_data[iter]),
            iter + 1, b_A_data, s_size + 1);
      }
      e[k - 1] = b_A_data[s_size];
    }
    for (int k{q + 1}; k < 4; k++) {
      s_size = (k + 3 * q) - 1;
      U_data[s_size] = b_A_data[s_size];
    }
    if (q <= 0) {
      nrm = blas::xnrm2(e);
      if (nrm == 0.0) {
        e[0] = 0.0;
      } else {
        if (e[1] < 0.0) {
          e[0] = -nrm;
        } else {
          e[0] = nrm;
        }
        nrm = e[0];
        if (std::abs(e[0]) >= 1.0020841800044864E-292) {
          nrm = 1.0 / e[0];
          for (int k{qp1}; k <= 2; k += 2) {
            r = _mm_loadu_pd(&e[1]);
            _mm_storeu_pd(&e[1], _mm_mul_pd(_mm_set1_pd(nrm), r));
          }
        } else {
          for (int k{qp1}; k <= 2; k += 2) {
            r = _mm_loadu_pd(&e[1]);
            _mm_storeu_pd(&e[1], _mm_div_pd(r, _mm_set1_pd(nrm)));
          }
        }
        e[1]++;
        e[0] = -e[0];
        for (int k{qp1}; k < 4; k++) {
          work_data[k - 1] = 0.0;
        }
        for (int k{qp1}; k < 4; k++) {
          blas::xaxpy(e[k - 1], b_A_data, 3 * (k - 1) + 2, work_data);
        }
        for (int k{qp1}; k < 4; k++) {
          blas::xaxpy(-e[k - 1] / e[1], work_data, b_A_data, 3 * (k - 1) + 2);
        }
      }
      for (int k{qp1}; k < 4; k++) {
        V[k - 1] = e[k - 1];
      }
    }
  }
  m = 2;
  b_s_data[2] = b_A_data[8];
  e[1] = b_A_data[7];
  e[2] = 0.0;
  U_data[6] = 0.0;
  U_data[7] = 0.0;
  U_data[8] = 1.0;
  for (int q{1}; q >= 0; q--) {
    qq = q + 3 * q;
    if (b_s_data[q] != 0.0) {
      for (int k{q + 2}; k < 4; k++) {
        s_size = (q + 3 * (k - 1)) + 1;
        blas::xaxpy(
            3 - q,
            -(blas::xdotc(3 - q, U_data, qq + 1, U_data, s_size) / U_data[qq]),
            qq + 1, U_data, s_size);
      }
      vectorUB = q + 3;
      s_size = q + 1;
      for (int k{q + 1}; k <= s_size; k += 2) {
        qs = (k + 3 * q) - 1;
        r = _mm_loadu_pd(&U_data[qs]);
        _mm_storeu_pd(&U_data[qs], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
      }
      for (int k{vectorUB}; k < 4; k++) {
        s_size = 3 * q + 2;
        U_data[s_size] = -U_data[s_size];
      }
      U_data[qq]++;
      if (q - 1 >= 0) {
        U_data[3 * q] = 0.0;
      }
    } else {
      U_data[3 * q] = 0.0;
      U_data[3 * q + 1] = 0.0;
      U_data[3 * q + 2] = 0.0;
      U_data[qq] = 1.0;
    }
  }
  for (int k{2}; k >= 0; k--) {
    if ((k <= 0) && (e[0] != 0.0)) {
      blas::b_xaxpy(2, -(blas::b_xdotc(2, V, 2, V, 5) / V[1]), 2, V, 5);
      blas::b_xaxpy(2, -(blas::b_xdotc(2, V, 2, V, 8) / V[1]), 2, V, 8);
    }
    V[3 * k] = 0.0;
    V[3 * k + 1] = 0.0;
    V[3 * k + 2] = 0.0;
    V[k + 3 * k] = 1.0;
  }
  iter = 0;
  snorm = 0.0;
  for (int q{0}; q < 3; q++) {
    nrm = b_s_data[q];
    if (nrm != 0.0) {
      rt = std::abs(nrm);
      nrm /= rt;
      b_s_data[q] = rt;
      if (q + 1 < 3) {
        e[q] /= nrm;
      }
      s_size = 3 * q + 1;
      std::copy(&U_data[0], &U_data[9], &x_data[0]);
      qs = s_size + 2;
      for (int k{s_size}; k <= s_size; k += 2) {
        r = _mm_loadu_pd(&x_data[k - 1]);
        _mm_storeu_pd(&x_data[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
      }
      for (int k{qs}; k <= s_size + 2; k++) {
        x_data[k - 1] *= nrm;
      }
      U_size[0] = 3;
      U_size[1] = 3;
      std::copy(&x_data[0], &x_data[9], &U_data[0]);
    }
    if (q + 1 < 3) {
      nrm = e[q];
      if (nrm != 0.0) {
        rt = std::abs(nrm);
        nrm = rt / nrm;
        e[q] = rt;
        b_s_data[q + 1] *= nrm;
        s_size = 3 * (q + 1) + 1;
        qs = s_size + 2;
        for (int k{s_size}; k <= s_size; k += 2) {
          r = _mm_loadu_pd(&V[k - 1]);
          _mm_storeu_pd(&V[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
        }
        for (int k{qs}; k <= s_size + 2; k++) {
          V[k - 1] *= nrm;
        }
      }
    }
    snorm = std::fmax(snorm, std::fmax(std::abs(b_s_data[q]), std::abs(e[q])));
  }
  while ((m + 1 > 0) && (iter < 75)) {
    boolean_T exitg1;
    vectorUB = m;
    exitg1 = false;
    while (!(exitg1 || (vectorUB == 0))) {
      nrm = std::abs(e[vectorUB - 1]);
      if ((nrm <= 2.220446049250313E-16 * (std::abs(b_s_data[vectorUB - 1]) +
                                           std::abs(b_s_data[vectorUB]))) ||
          (nrm <= 1.0020841800044864E-292) ||
          ((iter > 20) && (nrm <= 2.220446049250313E-16 * snorm))) {
        e[vectorUB - 1] = 0.0;
        exitg1 = true;
      } else {
        vectorUB--;
      }
    }
    if (vectorUB == m) {
      s_size = 4;
    } else {
      qs = m + 1;
      s_size = m + 1;
      exitg1 = false;
      while (!exitg1 && (s_size >= vectorUB)) {
        qs = s_size;
        if (s_size == vectorUB) {
          exitg1 = true;
        } else {
          nrm = 0.0;
          if (s_size < m + 1) {
            nrm = std::abs(e[s_size - 1]);
          }
          if (s_size > vectorUB + 1) {
            nrm += std::abs(e[s_size - 2]);
          }
          rt = std::abs(b_s_data[s_size - 1]);
          if ((rt <= 2.220446049250313E-16 * nrm) ||
              (rt <= 1.0020841800044864E-292)) {
            b_s_data[s_size - 1] = 0.0;
            exitg1 = true;
          } else {
            s_size--;
          }
        }
      }
      if (qs == vectorUB) {
        s_size = 3;
      } else if (qs == m + 1) {
        s_size = 1;
      } else {
        s_size = 2;
        vectorUB = qs;
      }
    }
    switch (s_size) {
    case 1:
      f = e[m - 1];
      e[m - 1] = 0.0;
      for (int k{m}; k >= vectorUB + 1; k--) {
        rt = blas::xrotg(b_s_data[k - 1], f, nrm);
        if (k > vectorUB + 1) {
          f = -nrm * e[0];
          e[0] *= rt;
        }
        blas::xrot(V, 3 * (k - 1) + 1, 3 * m + 1, rt, nrm);
      }
      break;
    case 2:
      f = e[vectorUB - 1];
      e[vectorUB - 1] = 0.0;
      for (int k{vectorUB + 1}; k <= m + 1; k++) {
        rt = blas::xrotg(b_s_data[k - 1], f, nrm);
        b = e[k - 1];
        f = -nrm * b;
        e[k - 1] = b * rt;
        blas::b_xrot(U_data, 3 * (k - 1) + 1, 3 * (vectorUB - 1) + 1, rt, nrm);
      }
      break;
    case 3: {
      double scale;
      nrm = b_s_data[m - 1];
      rt = e[m - 1];
      scale = std::fmax(
          std::fmax(std::fmax(std::fmax(std::abs(b_s_data[m]), std::abs(nrm)),
                              std::abs(rt)),
                    std::abs(b_s_data[vectorUB])),
          std::abs(e[vectorUB]));
      sm = b_s_data[m] / scale;
      nrm /= scale;
      rt /= scale;
      sqds = b_s_data[vectorUB] / scale;
      b = ((nrm + sm) * (nrm - sm) + rt * rt) / 2.0;
      nrm = sm * rt;
      nrm *= nrm;
      if ((b != 0.0) || (nrm != 0.0)) {
        rt = std::sqrt(b * b + nrm);
        if (b < 0.0) {
          rt = -rt;
        }
        rt = nrm / (b + rt);
      } else {
        rt = 0.0;
      }
      f = (sqds + sm) * (sqds - sm) + rt;
      nrm = sqds * (e[vectorUB] / scale);
      for (int k{vectorUB + 1}; k <= m; k++) {
        b = blas::xrotg(f, nrm, sm);
        if (k > vectorUB + 1) {
          e[0] = f;
        }
        nrm = e[k - 1];
        rt = b_s_data[k - 1];
        e[k - 1] = b * nrm - sm * rt;
        sqds = sm * b_s_data[k];
        b_s_data[k] *= b;
        s_size = 3 * (k - 1) + 1;
        qs = 3 * k + 1;
        blas::xrot(V, s_size, qs, b, sm);
        b_s_data[k - 1] = b * rt + sm * nrm;
        rt = blas::xrotg(b_s_data[k - 1], sqds, b);
        nrm = e[k - 1];
        f = rt * nrm + b * b_s_data[k];
        b_s_data[k] = -b * nrm + rt * b_s_data[k];
        nrm = b * e[k];
        e[k] *= rt;
        blas::b_xrot(U_data, s_size, qs, rt, b);
      }
      e[m - 1] = f;
      iter++;
    } break;
    default:
      if (b_s_data[vectorUB] < 0.0) {
        b_s_data[vectorUB] = -b_s_data[vectorUB];
        s_size = 3 * vectorUB + 1;
        qs = s_size + 2;
        for (int k{s_size}; k <= s_size; k += 2) {
          r = _mm_loadu_pd(&V[k - 1]);
          _mm_storeu_pd(&V[k - 1], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
        }
        for (int k{qs}; k <= s_size + 2; k++) {
          V[k - 1] = -V[k - 1];
        }
      }
      qp1 = vectorUB + 1;
      while ((vectorUB + 1 < 3) && (b_s_data[vectorUB] < b_s_data[qp1])) {
        rt = b_s_data[vectorUB];
        b_s_data[vectorUB] = b_s_data[qp1];
        b_s_data[qp1] = rt;
        qs = 3 * vectorUB + 1;
        s_size = 3 * (vectorUB + 1) + 1;
        blas::xswap(V, qs, s_size);
        std::copy(&U_data[0], &U_data[9], &x_data[0]);
        blas::b_xswap(x_data, qs, s_size);
        U_size[0] = 3;
        U_size[1] = 3;
        std::copy(&x_data[0], &x_data[9], &U_data[0]);
        vectorUB = qp1;
        qp1++;
      }
      iter = 0;
      m--;
      break;
    }
  }
  s_size = 3;
  s_data[0] = b_s_data[0];
  s_data[1] = b_s_data[1];
  s_data[2] = b_s_data[2];
  if (doscale) {
    reflapack::xzlascl(cscale, anrm, 3, s_data, 1);
  }
  return s_size;
}

void svd(const double A[9], double U[9], double s[3], double V[9])
{
  __m128d r;
  double b_A[9];
  double e[3];
  double work[3];
  double anrm;
  double b;
  double cscale;
  double f;
  double nrm;
  double rt;
  double sm;
  double snorm;
  double sqds;
  int iter;
  int m;
  int qjj;
  int qp1;
  int qq;
  int qs;
  int vectorUB;
  boolean_T doscale;
  s[0] = 0.0;
  e[0] = 0.0;
  work[0] = 0.0;
  s[1] = 0.0;
  e[1] = 0.0;
  work[1] = 0.0;
  s[2] = 0.0;
  e[2] = 0.0;
  work[2] = 0.0;
  for (int k{0}; k < 9; k++) {
    b_A[k] = A[k];
    U[k] = 0.0;
    V[k] = 0.0;
  }
  doscale = false;
  anrm = reflapack::b_xzlangeM(A);
  cscale = anrm;
  if ((anrm > 0.0) && (anrm < 6.717876107567089E-139)) {
    doscale = true;
    cscale = 6.717876107567089E-139;
    reflapack::b_xzlascl(anrm, cscale, b_A);
  } else if (anrm > 1.488565707357403E+138) {
    doscale = true;
    cscale = 1.488565707357403E+138;
    reflapack::b_xzlascl(anrm, cscale, b_A);
  }
  for (int q{0}; q < 2; q++) {
    boolean_T apply_transform;
    qp1 = q + 2;
    iter = q + 3 * q;
    qq = iter + 1;
    apply_transform = false;
    nrm = blas::c_xnrm2(3 - q, b_A, iter + 1);
    if (nrm > 0.0) {
      apply_transform = true;
      if (b_A[iter] < 0.0) {
        nrm = -nrm;
      }
      s[q] = nrm;
      if (std::abs(nrm) >= 1.0020841800044864E-292) {
        nrm = 1.0 / nrm;
        qjj = (iter - q) + 3;
        qs = ((((qjj - iter) / 2) << 1) + iter) + 1;
        vectorUB = qs - 2;
        for (int k{qq}; k <= vectorUB; k += 2) {
          r = _mm_loadu_pd(&b_A[k - 1]);
          _mm_storeu_pd(&b_A[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
        }
        for (int k{qs}; k <= qjj; k++) {
          b_A[k - 1] *= nrm;
        }
      } else {
        qjj = (iter - q) + 3;
        qs = ((((qjj - iter) / 2) << 1) + iter) + 1;
        vectorUB = qs - 2;
        for (int k{qq}; k <= vectorUB; k += 2) {
          r = _mm_loadu_pd(&b_A[k - 1]);
          _mm_storeu_pd(&b_A[k - 1], _mm_div_pd(r, _mm_set1_pd(s[q])));
        }
        for (int k{qs}; k <= qjj; k++) {
          b_A[k - 1] /= s[q];
        }
      }
      b_A[iter]++;
      s[q] = -s[q];
    } else {
      s[q] = 0.0;
    }
    for (int k{qp1}; k < 4; k++) {
      qjj = q + 3 * (k - 1);
      if (apply_transform) {
        blas::b_xaxpy(
            3 - q,
            -(blas::b_xdotc(3 - q, b_A, iter + 1, b_A, qjj + 1) / b_A[iter]),
            iter + 1, b_A, qjj + 1);
      }
      e[k - 1] = b_A[qjj];
    }
    for (int k{q + 1}; k < 4; k++) {
      qjj = (k + 3 * q) - 1;
      U[qjj] = b_A[qjj];
    }
    if (q <= 0) {
      nrm = blas::xnrm2(e);
      if (nrm == 0.0) {
        e[0] = 0.0;
      } else {
        if (e[1] < 0.0) {
          e[0] = -nrm;
        } else {
          e[0] = nrm;
        }
        nrm = e[0];
        if (std::abs(e[0]) >= 1.0020841800044864E-292) {
          nrm = 1.0 / e[0];
          for (int k{qp1}; k <= 2; k += 2) {
            r = _mm_loadu_pd(&e[1]);
            _mm_storeu_pd(&e[1], _mm_mul_pd(_mm_set1_pd(nrm), r));
          }
        } else {
          for (int k{qp1}; k <= 2; k += 2) {
            r = _mm_loadu_pd(&e[1]);
            _mm_storeu_pd(&e[1], _mm_div_pd(r, _mm_set1_pd(nrm)));
          }
        }
        e[1]++;
        e[0] = -e[0];
        for (int k{qp1}; k < 4; k++) {
          work[k - 1] = 0.0;
        }
        for (int k{qp1}; k < 4; k++) {
          blas::b_xaxpy(e[k - 1], b_A, 3 * (k - 1) + 2, work);
        }
        for (int k{qp1}; k < 4; k++) {
          blas::b_xaxpy(-e[k - 1] / e[1], work, b_A, 3 * (k - 1) + 2);
        }
      }
      for (int k{qp1}; k < 4; k++) {
        V[k - 1] = e[k - 1];
      }
    }
  }
  m = 2;
  s[2] = b_A[8];
  e[1] = b_A[7];
  e[2] = 0.0;
  U[6] = 0.0;
  U[7] = 0.0;
  U[8] = 1.0;
  for (int q{1}; q >= 0; q--) {
    qq = q + 3 * q;
    if (s[q] != 0.0) {
      for (int k{q + 2}; k < 4; k++) {
        qjj = (q + 3 * (k - 1)) + 1;
        blas::b_xaxpy(3 - q, -(blas::b_xdotc(3 - q, U, qq + 1, U, qjj) / U[qq]),
                      qq + 1, U, qjj);
      }
      vectorUB = q + 3;
      qjj = q + 1;
      for (int k{q + 1}; k <= qjj; k += 2) {
        qs = (k + 3 * q) - 1;
        r = _mm_loadu_pd(&U[qs]);
        _mm_storeu_pd(&U[qs], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
      }
      for (int k{vectorUB}; k < 4; k++) {
        qjj = 3 * q + 2;
        U[qjj] = -U[qjj];
      }
      U[qq]++;
      if (q - 1 >= 0) {
        U[3 * q] = 0.0;
      }
    } else {
      U[3 * q] = 0.0;
      U[3 * q + 1] = 0.0;
      U[3 * q + 2] = 0.0;
      U[qq] = 1.0;
    }
  }
  for (int k{2}; k >= 0; k--) {
    if ((k <= 0) && (e[0] != 0.0)) {
      blas::b_xaxpy(2, -(blas::b_xdotc(2, V, 2, V, 5) / V[1]), 2, V, 5);
      blas::b_xaxpy(2, -(blas::b_xdotc(2, V, 2, V, 8) / V[1]), 2, V, 8);
    }
    V[3 * k] = 0.0;
    V[3 * k + 1] = 0.0;
    V[3 * k + 2] = 0.0;
    V[k + 3 * k] = 1.0;
  }
  iter = 0;
  snorm = 0.0;
  for (int q{0}; q < 3; q++) {
    nrm = s[q];
    if (nrm != 0.0) {
      rt = std::abs(nrm);
      nrm /= rt;
      s[q] = rt;
      if (q + 1 < 3) {
        e[q] /= nrm;
      }
      qjj = 3 * q + 1;
      qs = qjj + 2;
      for (int k{qjj}; k <= qjj; k += 2) {
        r = _mm_loadu_pd(&U[k - 1]);
        _mm_storeu_pd(&U[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
      }
      for (int k{qs}; k <= qjj + 2; k++) {
        U[k - 1] *= nrm;
      }
    }
    if (q + 1 < 3) {
      nrm = e[q];
      if (nrm != 0.0) {
        rt = std::abs(nrm);
        nrm = rt / nrm;
        e[q] = rt;
        s[q + 1] *= nrm;
        qjj = 3 * (q + 1) + 1;
        qs = qjj + 2;
        for (int k{qjj}; k <= qjj; k += 2) {
          r = _mm_loadu_pd(&V[k - 1]);
          _mm_storeu_pd(&V[k - 1], _mm_mul_pd(_mm_set1_pd(nrm), r));
        }
        for (int k{qs}; k <= qjj + 2; k++) {
          V[k - 1] *= nrm;
        }
      }
    }
    snorm = std::fmax(snorm, std::fmax(std::abs(s[q]), std::abs(e[q])));
  }
  while ((m + 1 > 0) && (iter < 75)) {
    boolean_T exitg1;
    vectorUB = m;
    exitg1 = false;
    while (!(exitg1 || (vectorUB == 0))) {
      nrm = std::abs(e[vectorUB - 1]);
      if ((nrm <= 2.220446049250313E-16 *
                      (std::abs(s[vectorUB - 1]) + std::abs(s[vectorUB]))) ||
          (nrm <= 1.0020841800044864E-292) ||
          ((iter > 20) && (nrm <= 2.220446049250313E-16 * snorm))) {
        e[vectorUB - 1] = 0.0;
        exitg1 = true;
      } else {
        vectorUB--;
      }
    }
    if (vectorUB == m) {
      qjj = 4;
    } else {
      qs = m + 1;
      qjj = m + 1;
      exitg1 = false;
      while (!exitg1 && (qjj >= vectorUB)) {
        qs = qjj;
        if (qjj == vectorUB) {
          exitg1 = true;
        } else {
          nrm = 0.0;
          if (qjj < m + 1) {
            nrm = std::abs(e[qjj - 1]);
          }
          if (qjj > vectorUB + 1) {
            nrm += std::abs(e[qjj - 2]);
          }
          rt = std::abs(s[qjj - 1]);
          if ((rt <= 2.220446049250313E-16 * nrm) ||
              (rt <= 1.0020841800044864E-292)) {
            s[qjj - 1] = 0.0;
            exitg1 = true;
          } else {
            qjj--;
          }
        }
      }
      if (qs == vectorUB) {
        qjj = 3;
      } else if (qs == m + 1) {
        qjj = 1;
      } else {
        qjj = 2;
        vectorUB = qs;
      }
    }
    switch (qjj) {
    case 1:
      f = e[m - 1];
      e[m - 1] = 0.0;
      for (int k{m}; k >= vectorUB + 1; k--) {
        rt = blas::xrotg(s[k - 1], f, nrm);
        if (k > vectorUB + 1) {
          f = -nrm * e[0];
          e[0] *= rt;
        }
        blas::xrot(V, 3 * (k - 1) + 1, 3 * m + 1, rt, nrm);
      }
      break;
    case 2:
      f = e[vectorUB - 1];
      e[vectorUB - 1] = 0.0;
      for (int k{vectorUB + 1}; k <= m + 1; k++) {
        rt = blas::xrotg(s[k - 1], f, nrm);
        b = e[k - 1];
        f = -nrm * b;
        e[k - 1] = b * rt;
        blas::xrot(U, 3 * (k - 1) + 1, 3 * (vectorUB - 1) + 1, rt, nrm);
      }
      break;
    case 3: {
      double scale;
      nrm = s[m - 1];
      rt = e[m - 1];
      scale = std::fmax(
          std::fmax(
              std::fmax(std::fmax(std::abs(s[m]), std::abs(nrm)), std::abs(rt)),
              std::abs(s[vectorUB])),
          std::abs(e[vectorUB]));
      sm = s[m] / scale;
      nrm /= scale;
      rt /= scale;
      sqds = s[vectorUB] / scale;
      b = ((nrm + sm) * (nrm - sm) + rt * rt) / 2.0;
      nrm = sm * rt;
      nrm *= nrm;
      if ((b != 0.0) || (nrm != 0.0)) {
        rt = std::sqrt(b * b + nrm);
        if (b < 0.0) {
          rt = -rt;
        }
        rt = nrm / (b + rt);
      } else {
        rt = 0.0;
      }
      f = (sqds + sm) * (sqds - sm) + rt;
      nrm = sqds * (e[vectorUB] / scale);
      for (int k{vectorUB + 1}; k <= m; k++) {
        b = blas::xrotg(f, nrm, sm);
        if (k > vectorUB + 1) {
          e[0] = f;
        }
        nrm = e[k - 1];
        rt = s[k - 1];
        e[k - 1] = b * nrm - sm * rt;
        sqds = sm * s[k];
        s[k] *= b;
        qjj = 3 * (k - 1) + 1;
        qs = 3 * k + 1;
        blas::xrot(V, qjj, qs, b, sm);
        s[k - 1] = b * rt + sm * nrm;
        rt = blas::xrotg(s[k - 1], sqds, b);
        nrm = e[k - 1];
        f = rt * nrm + b * s[k];
        s[k] = -b * nrm + rt * s[k];
        nrm = b * e[k];
        e[k] *= rt;
        blas::xrot(U, qjj, qs, rt, b);
      }
      e[m - 1] = f;
      iter++;
    } break;
    default:
      if (s[vectorUB] < 0.0) {
        s[vectorUB] = -s[vectorUB];
        qjj = 3 * vectorUB + 1;
        qs = qjj + 2;
        for (int k{qjj}; k <= qjj; k += 2) {
          r = _mm_loadu_pd(&V[k - 1]);
          _mm_storeu_pd(&V[k - 1], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
        }
        for (int k{qs}; k <= qjj + 2; k++) {
          V[k - 1] = -V[k - 1];
        }
      }
      qp1 = vectorUB + 1;
      while ((vectorUB + 1 < 3) && (s[vectorUB] < s[qp1])) {
        rt = s[vectorUB];
        s[vectorUB] = s[qp1];
        s[qp1] = rt;
        qs = 3 * vectorUB + 1;
        qjj = 3 * (vectorUB + 1) + 1;
        blas::xswap(V, qs, qjj);
        blas::xswap(U, qs, qjj);
        vectorUB = qp1;
        qp1++;
      }
      iter = 0;
      m--;
      break;
    }
  }
  if (doscale) {
    reflapack::c_xzlascl(cscale, anrm, s);
  }
}

} // namespace internal
} // namespace coder

// End of code generation (svd.cpp)
