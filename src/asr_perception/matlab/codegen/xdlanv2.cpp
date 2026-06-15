//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xdlanv2.cpp
//
// Code generation for function 'xdlanv2'
//

// Include files
#include "xdlanv2.h"
#include "rt_nonfinite.h"
#include <cmath>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
double xdlanv2(double &a, double &b, double &c, double &d, double &rt1i,
               double &rt2r, double &rt2i, double &cs, double &sn)
{
  double rt1r;
  if (c == 0.0) {
    cs = 1.0;
    sn = 0.0;
  } else if (b == 0.0) {
    double temp;
    cs = 0.0;
    sn = 1.0;
    temp = d;
    d = a;
    a = temp;
    b = -c;
    c = 0.0;
  } else {
    double temp;
    temp = a - d;
    if ((temp == 0.0) && ((b < 0.0) != (c < 0.0))) {
      cs = 1.0;
      sn = 0.0;
    } else {
      double bcmax;
      double p;
      double scale;
      double tau;
      double z;
      int count;
      int i;
      p = 0.5 * temp;
      scale = std::abs(b);
      tau = std::abs(c);
      bcmax = std::fmax(scale, tau);
      if (!(b < 0.0)) {
        count = 1;
      } else {
        count = -1;
      }
      if (!(c < 0.0)) {
        i = 1;
      } else {
        i = -1;
      }
      rt1r = std::fmin(scale, tau) * static_cast<double>(count) *
             static_cast<double>(i);
      scale = std::fmax(std::abs(p), bcmax);
      z = p / scale * p + bcmax / scale * rt1r;
      if (z >= 8.881784197001252E-16) {
        scale = std::sqrt(scale) * std::sqrt(z);
        if (p < 0.0) {
          scale = -scale;
        }
        z = p + scale;
        a = d + z;
        d -= bcmax / z * rt1r;
        rt1r = std::abs(z);
        if (tau < rt1r) {
          scale = tau / rt1r;
          tau = rt1r * std::sqrt(scale * scale + 1.0);
        } else if (tau > rt1r) {
          rt1r /= tau;
          tau *= std::sqrt(rt1r * rt1r + 1.0);
        } else if (std::isnan(rt1r)) {
          tau = rtNaN;
        } else {
          tau *= 1.4142135623730951;
        }
        cs = z / tau;
        sn = c / tau;
        b -= c;
        c = 0.0;
      } else {
        z = b + c;
        scale = std::fmax(std::abs(temp), std::abs(z));
        count = 0;
        while ((scale >= 7.442828536787015E+137) && (count <= 20)) {
          z *= 1.3435752215134178E-138;
          temp *= 1.3435752215134178E-138;
          scale = std::fmax(std::abs(temp), std::abs(z));
          count++;
        }
        while ((scale <= 1.3435752215134178E-138) && (count <= 20)) {
          z *= 7.442828536787015E+137;
          temp *= 7.442828536787015E+137;
          scale = std::fmax(std::abs(temp), std::abs(z));
          count++;
        }
        bcmax = std::abs(z);
        scale = std::abs(temp);
        if (bcmax < scale) {
          rt1r = bcmax / scale;
          tau = scale * std::sqrt(rt1r * rt1r + 1.0);
        } else if (bcmax > scale) {
          scale /= bcmax;
          tau = bcmax * std::sqrt(scale * scale + 1.0);
        } else if (std::isnan(scale)) {
          tau = rtNaN;
        } else {
          tau = bcmax * 1.4142135623730951;
        }
        cs = std::sqrt(0.5 * (bcmax / tau + 1.0));
        if (!(z < 0.0)) {
          count = 1;
        } else {
          count = -1;
        }
        sn = -(0.5 * temp / (tau * cs)) * static_cast<double>(count);
        z = a * cs + b * sn;
        rt1r = -a * sn + b * cs;
        bcmax = c * cs + d * sn;
        scale = -c * sn + d * cs;
        b = rt1r * cs + scale * sn;
        c = -z * sn + bcmax * cs;
        temp = 0.5 * ((z * cs + bcmax * sn) + (-rt1r * sn + scale * cs));
        a = temp;
        d = temp;
        if (c != 0.0) {
          if (b != 0.0) {
            if ((b < 0.0) == (c < 0.0)) {
              scale = std::sqrt(std::abs(b));
              bcmax = std::sqrt(std::abs(c));
              p = scale * bcmax;
              if (c < 0.0) {
                p = -p;
              }
              tau = 1.0 / std::sqrt(std::abs(b + c));
              a = temp + p;
              d = temp - p;
              b -= c;
              c = 0.0;
              rt1r = scale * tau;
              scale = bcmax * tau;
              temp = cs * rt1r - sn * scale;
              sn = cs * scale + sn * rt1r;
              cs = temp;
            }
          } else {
            b = -c;
            c = 0.0;
            temp = cs;
            cs = -sn;
            sn = temp;
          }
        }
      }
    }
  }
  rt1r = a;
  rt2r = d;
  if (c == 0.0) {
    rt1i = 0.0;
    rt2i = 0.0;
  } else {
    rt1i = std::sqrt(std::abs(b)) * std::sqrt(std::abs(c));
    rt2i = -rt1i;
  }
  return rt1r;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xdlanv2.cpp)
