//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_detect_aruco_in_camera_frame_mex.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

// Include files
#include "_coder_detect_aruco_in_camera_frame_mex.h"
#include "_coder_detect_aruco_in_camera_frame_api.h"

// Function Definitions
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&detect_aruco_in_camera_frame_atexit);
  detect_aruco_in_camera_frame_initialize();
  unsafe_detect_aruco_in_camera_frame_mexFunction(nlhs, plhs, nrhs, prhs);
  detect_aruco_in_camera_frame_terminate();
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "UTF-8", true);
  return emlrtRootTLSGlobal;
}

void unsafe_detect_aruco_in_camera_frame_mexFunction(int32_T nlhs,
                                                     mxArray *plhs[4],
                                                     int32_T nrhs,
                                                     const mxArray *prhs[6])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *outputs[4];
  int32_T i;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 6) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 6, 4,
                        28, "detect_aruco_in_camera_frame");
  }
  if (nlhs > 4) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 28,
                        "detect_aruco_in_camera_frame");
  }
  // Call the function.
  detect_aruco_in_camera_frame_api(prhs, nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i = 1;
  } else {
    i = nlhs;
  }
  emlrtReturnArrays(i, &plhs[0], &outputs[0]);
}

// End of code generation (_coder_detect_aruco_in_camera_frame_mex.cpp)
