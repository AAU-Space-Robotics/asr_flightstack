//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// main.cpp
//
// Code generation for function 'main'
//

/*************************************************************************/
/* This automatically generated example C++ main file shows how to call  */
/* entry-point functions that MATLAB Coder generated. You must customize */
/* this file for your application. Do not modify this file directly.     */
/* Instead, make a copy of this file, modify it, and integrate it into   */
/* your development environment.                                         */
/*                                                                       */
/* This file initializes entry-point function arguments to a default     */
/* size and value before calling the entry-point functions. It does      */
/* not store or use any values returned from the entry-point functions.  */
/* If necessary, it does pre-allocate memory for returned values.        */
/* You can use this file as a starting point for a main function that    */
/* you can deploy in your application.                                   */
/*                                                                       */
/* After you copy the file, and before you deploy it, you must make the  */
/* following changes:                                                    */
/* * For variable-size function arguments, change the example sizes to   */
/* the sizes that your application requires.                             */
/* * Change the example values of function arguments to the values that  */
/* your application requires.                                            */
/* * If the entry-point functions return values, store these values or   */
/* otherwise use them as required by your application.                   */
/*                                                                       */
/*************************************************************************/

// Include files
#include "main.h"
#include "detect_aruco_in_camera_frame.h"
#include "detect_aruco_in_camera_frame_initialize.h"
#include "detect_aruco_in_camera_frame_terminate.h"
#include "rt_nonfinite.h"

// Function Declarations
static void argInit_480x640x3_uint8_T(unsigned char result[921600]);

static double argInit_real_T();

static unsigned char argInit_uint8_T();

// Function Definitions
static void argInit_480x640x3_uint8_T(unsigned char result[921600])
{
  // Loop over the array to initialize each element.
  for (int idx0{0}; idx0 < 480; idx0++) {
    for (int idx1{0}; idx1 < 640; idx1++) {
      for (int idx2{0}; idx2 < 3; idx2++) {
        // Set the value of the array element.
        // Change this value to the value that the application requires.
        result[(idx0 + 480 * idx1) + 307200 * idx2] = argInit_uint8_T();
      }
    }
  }
}

static double argInit_real_T()
{
  return 0.0;
}

static unsigned char argInit_uint8_T()
{
  return 0U;
}

int main(int, char **)
{
  // Initialize the application.
  // You do not need to do this more than one time.
  detect_aruco_in_camera_frame_initialize();
  // Invoke the entry-point functions.
  // You can call entry-point functions multiple times.
  main_detect_aruco_in_camera_frame();
  // Terminate the application.
  // You do not need to do this more than one time.
  detect_aruco_in_camera_frame_terminate();
  return 0;
}

void main_detect_aruco_in_camera_frame()
{
  static unsigned char uv[921600];
  double fx_tmp;
  float rvec[48];
  float tvec[48];
  int ids[16];
  int num_detections;
  // Initialize function 'detect_aruco_in_camera_frame' input arguments.
  // Initialize function input argument 'image_rgb'.
  fx_tmp = argInit_real_T();
  // Call the entry-point 'detect_aruco_in_camera_frame'.
  argInit_480x640x3_uint8_T(uv);
  detect_aruco_in_camera_frame(uv, fx_tmp, fx_tmp, fx_tmp, fx_tmp, fx_tmp, ids,
                               tvec, rvec, &num_detections);
}

// End of code generation (main.cpp)
