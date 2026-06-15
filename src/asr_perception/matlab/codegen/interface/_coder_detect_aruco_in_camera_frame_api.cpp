//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_detect_aruco_in_camera_frame_api.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

// Include files
#include "_coder_detect_aruco_in_camera_frame_api.h"
#include "_coder_detect_aruco_in_camera_frame_mex.h"

// Variable Definitions
emlrtCTX emlrtRootTLSGlobal{nullptr};

emlrtContext emlrtContextGlobal{
    true,                                                 // bFirstTime
    false,                                                // bInitialized
    131690U,                                              // fVersionInfo
    nullptr,                                              // fErrorFunction
    "detect_aruco_in_camera_frame",                       // fFunctionName
    nullptr,                                              // fRTCallStack
    false,                                                // bDebugMode
    {2045744189U, 2170104910U, 2743257031U, 4284093946U}, // fSigWrd
    nullptr                                               // fSigMem
};

// Function Declarations
static real_T b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *b_nullptr,
                                 const char_T *identifier);

static real_T b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                 const emlrtMsgIdentifier *parentId);

static uint8_T (*c_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                    const emlrtMsgIdentifier *msgId))[921600];

static real_T d_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                 const emlrtMsgIdentifier *msgId);

static void emlrtExitTimeCleanupDtorFcn(const void *r);

static uint8_T (*emlrt_marshallIn(const emlrtStack &sp,
                                  const mxArray *b_nullptr,
                                  const char_T *identifier))[921600];

static uint8_T (*emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                  const emlrtMsgIdentifier *parentId))[921600];

static const mxArray *emlrt_marshallOut(int32_T u[16]);

static const mxArray *emlrt_marshallOut(real32_T u[48]);

static const mxArray *emlrt_marshallOut(const int32_T u);

// Function Definitions
static real_T b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                 const emlrtMsgIdentifier *parentId)
{
  real_T y;
  y = d_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

static real_T b_emlrt_marshallIn(const emlrtStack &sp, const mxArray *b_nullptr,
                                 const char_T *identifier)
{
  emlrtMsgIdentifier thisId;
  real_T y;
  thisId.fIdentifier = const_cast<const char_T *>(identifier);
  thisId.fParent = nullptr;
  thisId.bParentIsCell = false;
  y = b_emlrt_marshallIn(sp, emlrtAlias(b_nullptr), &thisId);
  emlrtDestroyArray(&b_nullptr);
  return y;
}

static uint8_T (*c_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                    const emlrtMsgIdentifier *msgId))[921600]
{
  static const int32_T dims[3]{480, 640, 3};
  int32_T iv[3];
  uint8_T(*ret)[921600];
  boolean_T bv[3]{false, false, false};
  emlrtCheckVsBuiltInR2012b((emlrtConstCTX)&sp, msgId, src, "uint8", false, 3U,
                            (const void *)&dims[0], &bv[0], &iv[0]);
  ret = (uint8_T(*)[921600])emlrtMxGetData(src);
  emlrtDestroyArray(&src);
  return ret;
}

static real_T d_emlrt_marshallIn(const emlrtStack &sp, const mxArray *src,
                                 const emlrtMsgIdentifier *msgId)
{
  static const int32_T dims{0};
  real_T ret;
  emlrtCheckBuiltInR2012b((emlrtConstCTX)&sp, msgId, src, "double", false, 0U,
                          (const void *)&dims);
  ret = *static_cast<real_T *>(emlrtMxGetData(src));
  emlrtDestroyArray(&src);
  return ret;
}

static void emlrtExitTimeCleanupDtorFcn(const void *r)
{
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

static uint8_T (*emlrt_marshallIn(const emlrtStack &sp,
                                  const mxArray *b_nullptr,
                                  const char_T *identifier))[921600]
{
  emlrtMsgIdentifier thisId;
  uint8_T(*y)[921600];
  thisId.fIdentifier = const_cast<const char_T *>(identifier);
  thisId.fParent = nullptr;
  thisId.bParentIsCell = false;
  y = emlrt_marshallIn(sp, emlrtAlias(b_nullptr), &thisId);
  emlrtDestroyArray(&b_nullptr);
  return y;
}

static uint8_T (*emlrt_marshallIn(const emlrtStack &sp, const mxArray *u,
                                  const emlrtMsgIdentifier *parentId))[921600]
{
  uint8_T(*y)[921600];
  y = c_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

static const mxArray *emlrt_marshallOut(int32_T u[16])
{
  static const int32_T iv[2]{0, 0};
  static const int32_T iv1[2]{1, 16};
  const mxArray *m;
  const mxArray *y;
  void *existingData;
  y = nullptr;
  m = emlrtCreateNumericArray(2, (const void *)&iv[0], mxINT32_CLASS, mxREAL);
  existingData = emlrtMxGetData((mxArray *)m);
  if (existingData != (void *)&u[0]) {
    emlrtFreeMex(existingData);
  }
  emlrtMxSetData((mxArray *)m, &u[0]);
  emlrtSetDimensions((mxArray *)m, &iv1[0], 2);
  emlrtAssign(&y, m);
  return y;
}

static const mxArray *emlrt_marshallOut(real32_T u[48])
{
  static const int32_T iv[2]{0, 0};
  static const int32_T iv1[2]{3, 16};
  const mxArray *m;
  const mxArray *y;
  void *existingData;
  y = nullptr;
  m = emlrtCreateNumericArray(2, (const void *)&iv[0], mxSINGLE_CLASS, mxREAL);
  existingData = emlrtMxGetData((mxArray *)m);
  if (existingData != (void *)&u[0]) {
    emlrtFreeMex(existingData);
  }
  emlrtMxSetData((mxArray *)m, &u[0]);
  emlrtSetDimensions((mxArray *)m, &iv1[0], 2);
  emlrtAssign(&y, m);
  return y;
}

static const mxArray *emlrt_marshallOut(const int32_T u)
{
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
  *static_cast<int32_T *>(emlrtMxGetData(m)) = u;
  emlrtAssign(&y, m);
  return y;
}

void detect_aruco_in_camera_frame_api(const mxArray *const prhs[6],
                                      int32_T nlhs, const mxArray *plhs[4])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  real_T cx;
  real_T cy;
  real_T fx;
  real_T fy;
  real_T marker_size_mm;
  int32_T(*ids)[16];
  int32_T num_detections;
  real32_T(*rvec)[48];
  real32_T(*tvec)[48];
  uint8_T(*image_rgb)[921600];
  st.tls = emlrtRootTLSGlobal;
  ids = (int32_T(*)[16])mxMalloc(sizeof(int32_T[16]));
  tvec = (real32_T(*)[48])mxMalloc(sizeof(real32_T[48]));
  rvec = (real32_T(*)[48])mxMalloc(sizeof(real32_T[48]));
  // Marshall function inputs
  image_rgb = emlrt_marshallIn(st, emlrtAlias(prhs[0]), "image_rgb");
  fx = b_emlrt_marshallIn(st, emlrtAliasP(prhs[1]), "fx");
  fy = b_emlrt_marshallIn(st, emlrtAliasP(prhs[2]), "fy");
  cx = b_emlrt_marshallIn(st, emlrtAliasP(prhs[3]), "cx");
  cy = b_emlrt_marshallIn(st, emlrtAliasP(prhs[4]), "cy");
  marker_size_mm =
      b_emlrt_marshallIn(st, emlrtAliasP(prhs[5]), "marker_size_mm");
  // Invoke the target function
  detect_aruco_in_camera_frame(*image_rgb, fx, fy, cx, cy, marker_size_mm, *ids,
                               *tvec, *rvec, &num_detections);
  // Marshall function outputs
  plhs[0] = emlrt_marshallOut(*ids);
  if (nlhs > 1) {
    plhs[1] = emlrt_marshallOut(*tvec);
  }
  if (nlhs > 2) {
    plhs[2] = emlrt_marshallOut(*rvec);
  }
  if (nlhs > 3) {
    plhs[3] = emlrt_marshallOut(num_detections);
  }
}

void detect_aruco_in_camera_frame_atexit()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtPushHeapReferenceStackR2021a(&st, false, nullptr,
                                    (void *)&emlrtExitTimeCleanupDtorFcn,
                                    nullptr, nullptr, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
  detect_aruco_in_camera_frame_xil_terminate();
  detect_aruco_in_camera_frame_xil_shutdown();
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

void detect_aruco_in_camera_frame_initialize()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtClearAllocCountR2012b(&st, false, 0U, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtFirstTimeR2012b(emlrtRootTLSGlobal);
}

void detect_aruco_in_camera_frame_terminate()
{
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
}

// End of code generation (_coder_detect_aruco_in_camera_frame_api.cpp)
