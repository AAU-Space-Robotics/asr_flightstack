function [ids, tvec, rvec, num_detections] = detect_aruco_in_camera_frame( ...
        image_rgb, fx, fy, cx, cy, marker_size_mm)
%#codegen
% Detect ArUco markers (DICT_4X4_50) and return camera-frame poses.
%
% Inputs:
%   image_rgb      uint8 H×W×3  — RGB image
%   fx, fy         double       — focal lengths (pixels)
%   cx, cy         double       — principal point (pixels)
%   marker_size_mm double       — physical marker side length (mm)
%
% Outputs:
%   ids            int32  1×MAX_MARKERS  — detected marker IDs
%   tvec           single 3×MAX_MARKERS  — translation in camera frame (metres)
%   rvec           single 3×MAX_MARKERS  — Rodrigues rotation vector
%   num_detections int32  scalar

MAX_MARKERS = int32(16);

ids            = zeros(1,  MAX_MARKERS, 'int32');
tvec           = zeros(3,  MAX_MARKERS, 'single');
rvec           = zeros(3,  MAX_MARKERS, 'single');
num_detections = int32(0);

if ~isa(image_rgb, 'uint8') || size(image_rgb, 3) ~= 3
    return;
end

% Marker corner layout in object space (mm, z=0 plane)
half = marker_size_mm / 2;
objectPoints = single([ ...
    -half,  half, 0; ...
     half,  half, 0; ...
     half, -half, 0; ...
    -half, -half, 0]);

imageSize     = [size(image_rgb,1), size(image_rgb,2)];
camIntrinsics = cameraIntrinsics([fx fy], [cx cy], imageSize);

[detected_ids, locs] = readArucoMarker(image_rgb, ...
    'RefineCorners',           true, ...
    'ResolutionPerBit',        20, ...
    'WindowSizeStep',          2, ...
    'WindowSizeRange',         [3 10], ...
    'RefinementMaxIterations', 100, ...
    'RefinementTolerance',     0.01);

if isempty(detected_ids)
    return;
end

n = min(int32(numel(detected_ids)), MAX_MARKERS);

for i = 1:n
    ids(i) = int32(detected_ids(i));

    imagePoints = single(squeeze(locs(:,:,i)));

    [worldOrientation, worldLocation] = estimateWorldCameraPose( ...
        imagePoints, objectPoints, camIntrinsics, ...
        'MaxReprojectionError', 3, ...
        'Confidence',           98, ...
        'MaxNumTrials',         2000);

    R = single(worldOrientation');
    t = single(-worldLocation * double(R));   % camera-frame translation (mm)
    tvec(:,i) = single(t(:)) / 1000;          % convert mm → metres
    rvec(:,i) = single(rotationMatrixToVector(double(R)))';
end

num_detections = n;
end
