#!/bin/bash
# Run this once on the MATLAB ground station after generating codegen output.
# Copies the MATLAB Computer Vision Toolbox C++ sources and headers that are
# needed to compile aruco_detector on machines without MATLAB.
set -e

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
MATLAB_ROOT=${MATLAB_ROOT:-/usr/local/MATLAB/R2026a}
OCV_DIR=$MATLAB_ROOT/toolbox/vision/builtins/src/ocv
VENDOR_DIR=$SCRIPT_DIR/vendor

if [ ! -d "$MATLAB_ROOT" ]; then
  echo "ERROR: MATLAB not found at $MATLAB_ROOT"
  echo "Set MATLAB_ROOT env var to your MATLAB installation, e.g.:"
  echo "  MATLAB_ROOT=/usr/local/MATLAB/R2025b $0"
  exit 1
fi

mkdir -p "$VENDOR_DIR/src/aruco"
mkdir -p "$VENDOR_DIR/include/aruco"

cp "$OCV_DIR/readArucoMarkerCore.cpp"   "$VENDOR_DIR/src/"
cp "$OCV_DIR/cgCommon.cpp"              "$VENDOR_DIR/src/"
cp "$OCV_DIR/aruco/readArucoImpl.cpp"   "$VENDOR_DIR/src/aruco/"
cp "$OCV_DIR/aruco/arucoDictionary.cpp" "$VENDOR_DIR/src/aruco/"

cp -r "$OCV_DIR/include/." "$VENDOR_DIR/include/"

# tmwtypes.h is included transitively by the toolbox sources
cp "$MATLAB_ROOT/extern/include/tmwtypes.h" "$VENDOR_DIR/include/"

echo "Done — commit matlab/vendor/ alongside matlab/codegen/"
echo "The UAV can now build aruco_detector without MATLAB using system OpenCV."
