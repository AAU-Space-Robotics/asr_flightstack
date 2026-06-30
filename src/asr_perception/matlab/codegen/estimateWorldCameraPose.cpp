//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// estimateWorldCameraPose.cpp
//
// Code generation for function 'estimateWorldCameraPose'
//

// Include files
#include "estimateWorldCameraPose.h"
#include "cameraIntrinsics.h"
#include "detect_aruco_in_camera_frame_rtwutil.h"
#include "rand.h"
#include "rt_nonfinite.h"
#include "solveP3P.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
void estimateWorldCameraPose(const float imagePoints_data[],
                             const float worldPoints[12],
                             const cameraIntrinsics &cameraParams,
                             double orientation[9], double location[3])
{
  double Rs_data[36];
  double points_data[20];
  double b_points_data[16];
  double Rs[12];
  double Ts_data[12];
  double projectedPointsHomog[12];
  double modelParams_R[9];
  double s_R[9];
  double projectedPoints[8];
  double y[8];
  double errors_data[4];
  double indices[4];
  double s_t[3];
  double bestDis;
  double j;
  double p_data_idx_1;
  double p_data_idx_2;
  int b_points_data_tmp;
  int idxTrial;
  int numTrials;
  int points_data_tmp;
  boolean_T bestInliers_idx_0;
  boolean_T bestInliers_idx_1;
  boolean_T bestInliers_idx_2;
  boolean_T bestInliers_idx_3;
  for (int k{0}; k < 2; k++) {
    points_data[4 * k] = imagePoints_data[4 * k];
    points_data_tmp = 4 * k + 1;
    points_data[points_data_tmp] = imagePoints_data[points_data_tmp];
    points_data_tmp = 4 * k + 2;
    points_data[points_data_tmp] = imagePoints_data[points_data_tmp];
    points_data_tmp = 4 * k + 3;
    points_data[points_data_tmp] = imagePoints_data[points_data_tmp];
  }
  for (int k{0}; k < 3; k++) {
    points_data_tmp = k << 2;
    b_points_data_tmp = 4 * (k + 2);
    points_data[b_points_data_tmp] = worldPoints[points_data_tmp];
    points_data[b_points_data_tmp + 1] = worldPoints[points_data_tmp + 1];
    points_data[b_points_data_tmp + 2] = worldPoints[points_data_tmp + 2];
    points_data[b_points_data_tmp + 3] = worldPoints[points_data_tmp + 3];
    orientation[3 * k] = cameraParams.K[k];
    orientation[3 * k + 1] = cameraParams.K[k + 3];
    orientation[3 * k + 2] = cameraParams.K[k + 6];
  }
  idxTrial = 1;
  numTrials = 2000;
  bestDis = 36.0;
  for (int k{0}; k < 9; k++) {
    s_R[k] = rtNaN;
  }
  s_t[0] = rtNaN;
  s_t[1] = rtNaN;
  s_t[2] = rtNaN;
  bestInliers_idx_0 = false;
  bestInliers_idx_1 = false;
  bestInliers_idx_2 = false;
  bestInliers_idx_3 = false;
  while (idxTrial <= numTrials) {
    __m128d r;
    __m128d r1;
    __m128d r2;
    double samplePoints_data[20];
    int Rs_size[3];
    int Ts_size[2];
    int iindx;
    int projectedPointsHomog_tmp;
    indices[1] = 0.0;
    indices[2] = 0.0;
    indices[3] = 0.0;
    indices[0] = 1.0;
    j = b_rand() * 2.0;
    j = std::floor(j);
    indices[1] = indices[static_cast<int>(j + 1.0) - 1];
    indices[static_cast<int>(j + 1.0) - 1] = 2.0;
    j = b_rand() * 3.0;
    j = std::floor(j);
    indices[2] = indices[static_cast<int>(j + 1.0) - 1];
    indices[static_cast<int>(j + 1.0) - 1] = 3.0;
    j = b_rand() * 4.0;
    j = std::floor(j);
    indices[3] = indices[static_cast<int>(j + 1.0) - 1];
    indices[static_cast<int>(j + 1.0) - 1] = 4.0;
    points_data_tmp = static_cast<int>(indices[0]);
    b_points_data_tmp = static_cast<int>(indices[1]);
    projectedPointsHomog_tmp = static_cast<int>(indices[2]);
    iindx = static_cast<int>(indices[3]);
    for (int i{0}; i < 5; i++) {
      samplePoints_data[4 * i] = points_data[(points_data_tmp + 4 * i) - 1];
      samplePoints_data[4 * i + 1] =
          points_data[(b_points_data_tmp + 4 * i) - 1];
      samplePoints_data[4 * i + 2] =
          points_data[(projectedPointsHomog_tmp + 4 * i) - 1];
      samplePoints_data[4 * i + 3] = points_data[(iindx + 4 * i) - 1];
    }
    for (int k{0}; k < 3; k++) {
      points_data_tmp = 4 * (k + 2);
      projectedPointsHomog[4 * k] = samplePoints_data[points_data_tmp];
      projectedPointsHomog[4 * k + 1] = samplePoints_data[points_data_tmp + 1];
      projectedPointsHomog[4 * k + 2] = samplePoints_data[points_data_tmp + 2];
      projectedPointsHomog[4 * k + 3] = samplePoints_data[points_data_tmp + 3];
    }
    vision::internal::calibration::solveP3P(&samplePoints_data[0],
                                            projectedPointsHomog, orientation,
                                            Rs_data, Rs_size, Ts_data, Ts_size);
    j = samplePoints_data[11];
    p_data_idx_1 = samplePoints_data[15];
    p_data_idx_2 = samplePoints_data[19];
    for (int k{0}; k < 9; k++) {
      modelParams_R[k] = rtNaN;
    }
    location[0] = rtNaN;
    location[1] = rtNaN;
    location[2] = rtNaN;
    if (Rs_size[2] != 0) {
      int last;
      last = Ts_size[0];
      for (int b_i{0}; b_i < last; b_i++) {
        points_data_tmp = Ts_size[0];
        for (int k{0}; k < 3; k++) {
          b_points_data_tmp = k << 2;
          projectedPointsHomog_tmp = 3 * k + 9 * b_i;
          projectedPointsHomog[b_points_data_tmp] =
              Rs_data[projectedPointsHomog_tmp];
          projectedPointsHomog[b_points_data_tmp + 1] =
              Rs_data[projectedPointsHomog_tmp + 1];
          projectedPointsHomog[b_points_data_tmp + 2] =
              Rs_data[projectedPointsHomog_tmp + 2];
          projectedPointsHomog[b_points_data_tmp + 3] =
              Ts_data[b_i + points_data_tmp * k];
        }
        std::memset(&Rs[0], 0, 12U * sizeof(double));
        std::memset(&location[0], 0, 3U * sizeof(double));
        for (int i{0}; i < 3; i++) {
          points_data_tmp = i << 2;
          for (int k{0}; k < 3; k++) {
            b_points_data_tmp = k << 2;
            r = _mm_loadu_pd(&projectedPointsHomog[b_points_data_tmp]);
            r1 = _mm_loadu_pd(&Rs[points_data_tmp]);
            r2 = _mm_set1_pd(orientation[k + 3 * i]);
            _mm_storeu_pd(&Rs[points_data_tmp],
                          _mm_add_pd(r1, _mm_mul_pd(r, r2)));
            r = _mm_loadu_pd(&projectedPointsHomog[b_points_data_tmp + 2]);
            r1 = _mm_loadu_pd(&Rs[points_data_tmp + 2]);
            _mm_storeu_pd(&Rs[points_data_tmp + 2],
                          _mm_add_pd(r1, _mm_mul_pd(r, r2)));
          }
          points_data_tmp = i << 2;
          location[i] = (((location[i] + j * Rs[points_data_tmp]) +
                          p_data_idx_1 * Rs[points_data_tmp + 1]) +
                         p_data_idx_2 * Rs[points_data_tmp + 2]) +
                        Rs[points_data_tmp + 3];
        }
        double b_d;
        double d;
        d = samplePoints_data[3] - location[0] / location[2];
        b_d = d * d;
        d = samplePoints_data[7] - location[1] / location[2];
        b_d += d * d;
        errors_data[b_i] = b_d;
      }
      if (Ts_size[0] <= 2) {
        if (Ts_size[0] == 1) {
          iindx = 1;
        } else {
          j = errors_data[Ts_size[0] - 1];
          if ((errors_data[0] > j) ||
              (std::isnan(errors_data[0]) && !std::isnan(j))) {
            iindx = Ts_size[0];
          } else {
            iindx = 1;
          }
        }
      } else {
        if (!std::isnan(errors_data[0])) {
          b_points_data_tmp = 1;
        } else {
          boolean_T exitg1;
          b_points_data_tmp = 0;
          points_data_tmp = 2;
          exitg1 = false;
          while (!exitg1 && (points_data_tmp <= last)) {
            if (!std::isnan(errors_data[points_data_tmp - 1])) {
              b_points_data_tmp = points_data_tmp;
              exitg1 = true;
            } else {
              points_data_tmp++;
            }
          }
        }
        if (b_points_data_tmp == 0) {
          iindx = 1;
        } else {
          j = errors_data[b_points_data_tmp - 1];
          iindx = b_points_data_tmp;
          for (int k{b_points_data_tmp + 1}; k <= last; k++) {
            p_data_idx_1 = errors_data[k - 1];
            if (j > p_data_idx_1) {
              j = p_data_idx_1;
              iindx = k;
            }
          }
        }
      }
      points_data_tmp = Ts_size[0];
      for (int k{0}; k < 3; k++) {
        location[k] = Ts_data[(iindx + points_data_tmp * k) - 1];
        b_points_data_tmp = 3 * k + 9 * (iindx - 1);
        modelParams_R[3 * k] = Rs_data[b_points_data_tmp];
        modelParams_R[3 * k + 1] = Rs_data[b_points_data_tmp + 1];
        modelParams_R[3 * k + 2] = Rs_data[b_points_data_tmp + 2];
      }
    }
    for (int k{0}; k < 3; k++) {
      points_data_tmp = 4 * (k + 2);
      b_points_data[4 * k] = points_data[points_data_tmp];
      b_points_data[4 * k + 1] = points_data[points_data_tmp + 1];
      b_points_data[4 * k + 2] = points_data[points_data_tmp + 2];
      b_points_data[4 * k + 3] = points_data[points_data_tmp + 3];
    }
    b_points_data[12] = 1.0;
    b_points_data[13] = 1.0;
    b_points_data[14] = 1.0;
    b_points_data[15] = 1.0;
    for (int k{0}; k < 3; k++) {
      points_data_tmp = k << 2;
      projectedPointsHomog[points_data_tmp] = modelParams_R[3 * k];
      projectedPointsHomog[points_data_tmp + 1] = modelParams_R[3 * k + 1];
      projectedPointsHomog[points_data_tmp + 2] = modelParams_R[3 * k + 2];
      projectedPointsHomog[points_data_tmp + 3] = location[k];
    }
    std::memset(&Rs[0], 0, 12U * sizeof(double));
    for (int k{0}; k < 3; k++) {
      points_data_tmp = k << 2;
      for (int i{0}; i < 3; i++) {
        b_points_data_tmp = i << 2;
        r = _mm_loadu_pd(&projectedPointsHomog[b_points_data_tmp]);
        r1 = _mm_loadu_pd(&Rs[points_data_tmp]);
        r2 = _mm_set1_pd(orientation[i + 3 * k]);
        _mm_storeu_pd(&Rs[points_data_tmp], _mm_add_pd(r1, _mm_mul_pd(r, r2)));
        r = _mm_loadu_pd(&projectedPointsHomog[b_points_data_tmp + 2]);
        r1 = _mm_loadu_pd(&Rs[points_data_tmp + 2]);
        _mm_storeu_pd(&Rs[points_data_tmp + 2],
                      _mm_add_pd(r1, _mm_mul_pd(r, r2)));
      }
    }
    std::memset(&projectedPointsHomog[0], 0, 12U * sizeof(double));
    for (int k{0}; k < 3; k++) {
      points_data_tmp = k << 2;
      for (int i{0}; i < 4; i++) {
        r = _mm_loadu_pd(&b_points_data[4 * i]);
        r1 = _mm_loadu_pd(&projectedPointsHomog[points_data_tmp]);
        r2 = _mm_set1_pd(Rs[i + points_data_tmp]);
        _mm_storeu_pd(&projectedPointsHomog[points_data_tmp],
                      _mm_add_pd(r1, _mm_mul_pd(r, r2)));
        r = _mm_loadu_pd(&b_points_data[4 * i + 2]);
        r1 = _mm_loadu_pd(&projectedPointsHomog[points_data_tmp + 2]);
        _mm_storeu_pd(&projectedPointsHomog[points_data_tmp + 2],
                      _mm_add_pd(r1, _mm_mul_pd(r, r2)));
      }
    }
    r = _mm_loadu_pd(&projectedPointsHomog[0]);
    r1 = _mm_loadu_pd(&projectedPointsHomog[8]);
    r2 = _mm_loadu_pd(&points_data[0]);
    _mm_storeu_pd(&projectedPoints[0], _mm_sub_pd(r2, _mm_div_pd(r, r1)));
    r = _mm_loadu_pd(&projectedPointsHomog[2]);
    r1 = _mm_loadu_pd(&projectedPointsHomog[10]);
    r2 = _mm_loadu_pd(&points_data[2]);
    _mm_storeu_pd(&projectedPoints[2], _mm_sub_pd(r2, _mm_div_pd(r, r1)));
    r = _mm_loadu_pd(&projectedPointsHomog[4]);
    r1 = _mm_loadu_pd(&projectedPointsHomog[8]);
    r2 = _mm_loadu_pd(&points_data[4]);
    _mm_storeu_pd(&projectedPoints[4], _mm_sub_pd(r2, _mm_div_pd(r, r1)));
    r = _mm_loadu_pd(&projectedPointsHomog[6]);
    r1 = _mm_loadu_pd(&projectedPointsHomog[10]);
    r2 = _mm_loadu_pd(&points_data[6]);
    _mm_storeu_pd(&projectedPoints[6], _mm_sub_pd(r2, _mm_div_pd(r, r1)));
    r = _mm_loadu_pd(&projectedPoints[0]);
    _mm_storeu_pd(&y[0], _mm_mul_pd(r, r));
    r = _mm_loadu_pd(&projectedPoints[2]);
    _mm_storeu_pd(&y[2], _mm_mul_pd(r, r));
    r = _mm_loadu_pd(&projectedPoints[4]);
    _mm_storeu_pd(&y[4], _mm_mul_pd(r, r));
    r = _mm_loadu_pd(&projectedPoints[6]);
    _mm_storeu_pd(&y[6], _mm_mul_pd(r, r));
    r = _mm_loadu_pd(&y[0]);
    r1 = _mm_loadu_pd(&y[4]);
    _mm_storeu_pd(&indices[0], _mm_add_pd(r, r1));
    r = _mm_loadu_pd(&y[2]);
    r1 = _mm_loadu_pd(&y[6]);
    _mm_storeu_pd(&indices[2], _mm_add_pd(r, r1));
    if (indices[0] > 9.0) {
      indices[0] = 9.0;
    }
    if (indices[1] > 9.0) {
      indices[1] = 9.0;
    }
    if (indices[2] > 9.0) {
      indices[2] = 9.0;
    }
    if (indices[3] > 9.0) {
      indices[3] = 9.0;
    }
    j = ((indices[0] + indices[1]) + indices[2]) + indices[3];
    if (j < bestDis) {
      bestDis = j;
      bestInliers_idx_0 = (indices[0] < 9.0);
      bestInliers_idx_1 = (indices[1] < 9.0);
      bestInliers_idx_2 = (indices[2] < 9.0);
      bestInliers_idx_3 = (indices[3] < 9.0);
      std::copy(&modelParams_R[0], &modelParams_R[9], &s_R[0]);
      s_t[0] = location[0];
      s_t[1] = location[1];
      s_t[2] = location[2];
      j = rt_powd_snf((((static_cast<double>(bestInliers_idx_0) +
                         static_cast<double>(bestInliers_idx_1)) +
                        static_cast<double>(bestInliers_idx_2)) +
                       static_cast<double>(bestInliers_idx_3)) /
                          4.0,
                      4.0);
      if (j < 2.220446049250313E-16) {
        points_data_tmp = MAX_int32_T;
      } else {
        j = std::ceil(-1.6989700043360183 / std::log10(1.0 - j));
        if (j < 2.147483648E+9) {
          points_data_tmp = static_cast<int>(j);
        } else {
          points_data_tmp = MAX_int32_T;
        }
      }
      if (numTrials > points_data_tmp) {
        numTrials = points_data_tmp;
      }
    }
    idxTrial++;
  }
  if (((bestInliers_idx_0 + bestInliers_idx_1) + bestInliers_idx_2) +
          bestInliers_idx_3 >=
      4) {
    for (int k{0}; k < 3; k++) {
      orientation[3 * k] = s_R[k];
      orientation[3 * k + 1] = s_R[k + 3];
      orientation[3 * k + 2] = s_R[k + 6];
      s_t[k] = -s_t[k];
    }
    std::memset(&location[0], 0, 3U * sizeof(double));
    j = s_t[0];
    p_data_idx_1 = s_t[1];
    p_data_idx_2 = s_t[2];
    for (int k{0}; k < 3; k++) {
      location[k] = ((location[k] + j * orientation[3 * k]) +
                     p_data_idx_1 * orientation[3 * k + 1]) +
                    p_data_idx_2 * orientation[3 * k + 2];
    }
  } else {
    for (int k{0}; k < 9; k++) {
      orientation[k] = rtNaN;
    }
    location[0] = rtNaN;
    location[1] = rtNaN;
    location[2] = rtNaN;
  }
}

} // namespace coder

// End of code generation (estimateWorldCameraPose.cpp)
