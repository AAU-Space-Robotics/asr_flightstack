//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_detect_aruco_in_camera_frame_info.cpp
//
// Code generation for function 'detect_aruco_in_camera_frame'
//

// Include files
#include "_coder_detect_aruco_in_camera_frame_info.h"
#include "emlrt.h"
#include "tmwtypes.h"

// Function Declarations
static const mxArray *emlrtMexFcnResolvedFunctionsInfo();

// Function Definitions
static const mxArray *emlrtMexFcnResolvedFunctionsInfo()
{
  const mxArray *nameCaptureInfo;
  const char_T *data[6]{
      "789ced55bf6fd340143e5b4949255a8a902a0606063624cc98bd05b5a205442a5181907b"
      "752e8d897fe97c8674eb7f00232363473618d9e89fc0c2ff51317189"
      "fd1ce7c9af97462265e0494fe7cfdfddfb9edf9ddf316b7bd7628cadb2dcce96f271a5c0"
      "6bc568b369c3bc458c604dd6985a07fc8762f4e24889a1ca41c44351",
      "aef4a324537bc789483590228d8377a23b667a7e20f6fc5074aae0e908858f2b540946d4"
      "e879a32fbc41270b99eca7930c832a28eb71407c6fc3500f6cb81e78"
      "1ee89dcca907f1ef18f480ef0a253ce5729979b1eb47aea74b2eb9db9345e9219f9f84de"
      "4d141feb5945c6bb1bcf3a39ce0f56aaa41f1d4de20f89f88019c12f",
      "215dac0ffcedc9ab5f7631afeae5b9277466adbb4098a179c0bf7ef4c6e9c7a170ba5cc6"
      "c2e1a974dfc7729026dccb512ff08ffa2a55dc1b38a9f4c6ef12213d"
      "91283f8e9c90ab801f3a176ddf839099cfef8d19bf8baaef0a6b8dc7875fefbf650bd41b"
      "fcf87d6f917a6057a547fd1fb39ecb75426f0df1ca7b2142b539dcea",
      "0cdb9b3bead5936c47c8ad491ecf0d3aa63c188117157fbf55bf7e79c6f8e7d7eae3db88"
      "670bbe4f97110f18fa2e18e4d326f2b97c3f9f8e33ba575f4a9ee856"
      "e5ba15bdb366bd9e695f4dfd1de2dfb5ead79bf6f5bb21be8de6358a776dedfbda13ed9f"
      "b59f56fc8b761e1d97771c6578ef411ffc22b388e7ba79df2a7815f1",
      "80cf8b71de7e768bc805e67f42bc8d30ec731361d398319f454ce9fde8b258a34316e83b"
      "f6b2cf95fe439c23d339c5df439dd3c4b09e8a6fea6fd007fe75fd03"
      "22fe7583bea9be4de2defcbf9f7f57bf3567df8578a7447c1bf1f83efd48c4bdaafb14ec"
      "0f1817a65f",
      ""};
  nameCaptureInfo = nullptr;
  emlrtNameCaptureMxArrayR2016a(&data[0], 3912U, &nameCaptureInfo);
  return nameCaptureInfo;
}

mxArray *emlrtMexFcnProperties()
{
  mxArray *xEntryPoints;
  mxArray *xInputs;
  mxArray *xResult;
  const char_T *epFieldName[7]{
      "QualifiedName",    "NumberOfInputs", "NumberOfOutputs", "ConstantInputs",
      "ResolvedFilePath", "TimeStamp",      "Visible"};
  const char_T *propFieldName[7]{
      "Version",      "ResolvedFunctions", "Checksum", "EntryPoints",
      "CoverageInfo", "IsPolymorphic",     "AuxData"};
  uint8_T v[216]{
      0U,   1U,   73U,  77U,  0U,   0U,   0U,   0U,   14U,  0U,   0U,   0U,
      200U, 0U,   0U,   0U,   6U,   0U,   0U,   0U,   8U,   0U,   0U,   0U,
      2U,   0U,   0U,   0U,   0U,   0U,   0U,   0U,   5U,   0U,   0U,   0U,
      8U,   0U,   0U,   0U,   1U,   0U,   0U,   0U,   1U,   0U,   0U,   0U,
      1U,   0U,   0U,   0U,   0U,   0U,   0U,   0U,   5U,   0U,   4U,   0U,
      17U,  0U,   0U,   0U,   1U,   0U,   0U,   0U,   17U,  0U,   0U,   0U,
      67U,  108U, 97U,  115U, 115U, 69U,  110U, 116U, 114U, 121U, 80U,  111U,
      105U, 110U, 116U, 115U, 0U,   0U,   0U,   0U,   0U,   0U,   0U,   0U,
      14U,  0U,   0U,   0U,   112U, 0U,   0U,   0U,   6U,   0U,   0U,   0U,
      8U,   0U,   0U,   0U,   2U,   0U,   0U,   0U,   0U,   0U,   0U,   0U,
      5U,   0U,   0U,   0U,   8U,   0U,   0U,   0U,   1U,   0U,   0U,   0U,
      0U,   0U,   0U,   0U,   1U,   0U,   0U,   0U,   0U,   0U,   0U,   0U,
      5U,   0U,   4U,   0U,   14U,  0U,   0U,   0U,   1U,   0U,   0U,   0U,
      56U,  0U,   0U,   0U,   81U,  117U, 97U,  108U, 105U, 102U, 105U, 101U,
      100U, 78U,  97U,  109U, 101U, 0U,   77U,  101U, 116U, 104U, 111U, 100U,
      115U, 0U,   0U,   0U,   0U,   0U,   0U,   0U,   80U,  114U, 111U, 112U,
      101U, 114U, 116U, 105U, 101U, 115U, 0U,   0U,   0U,   0U,   72U,  97U,
      110U, 100U, 108U, 101U, 0U,   0U,   0U,   0U,   0U,   0U,   0U,   0U};
  xEntryPoints =
      emlrtCreateStructMatrix(1, 1, 7, (const char_T **)&epFieldName[0]);
  xInputs = emlrtCreateLogicalMatrix(1, 6);
  emlrtSetField(xEntryPoints, 0, "QualifiedName",
                emlrtMxCreateString("detect_aruco_in_camera_frame"));
  emlrtSetField(xEntryPoints, 0, "NumberOfInputs",
                emlrtMxCreateDoubleScalar(6.0));
  emlrtSetField(xEntryPoints, 0, "NumberOfOutputs",
                emlrtMxCreateDoubleScalar(4.0));
  emlrtSetField(xEntryPoints, 0, "ConstantInputs", xInputs);
  emlrtSetField(xEntryPoints, 0, "ResolvedFilePath",
                emlrtMxCreateString(
                    "/home/daroe/asr_workspace/asr_flightstack/src/"
                    "asr_perception/matlab/detect_aruco_in_camera_frame.m"));
  emlrtSetField(xEntryPoints, 0, "TimeStamp",
                emlrtMxCreateDoubleScalar(740145.4009259259));
  emlrtSetField(xEntryPoints, 0, "Visible", emlrtMxCreateLogicalScalar(true));
  xResult =
      emlrtCreateStructMatrix(1, 1, 7, (const char_T **)&propFieldName[0]);
  emlrtSetField(xResult, 0, "Version",
                emlrtMxCreateString("26.1.0.3251617 (R2026a) Update 2"));
  emlrtSetField(xResult, 0, "ResolvedFunctions",
                (mxArray *)emlrtMexFcnResolvedFunctionsInfo());
  emlrtSetField(xResult, 0, "Checksum",
                emlrtMxCreateString("jbCPRX9zMpP3ZLyb5hObkD"));
  emlrtSetField(xResult, 0, "EntryPoints", xEntryPoints);
  emlrtSetField(xResult, 0, "AuxData",
                emlrtMxCreateRowVectorUINT8((const uint8_T *)&v, 216U));
  return xResult;
}

// End of code generation (_coder_detect_aruco_in_camera_frame_info.cpp)
