% generate_code.m — converts detect_aruco_in_camera_frame.m to C++.
% Run from anywhere:
%   matlab -batch "run('/path/to/matlab/generate_code.m')"
% or from the matlab/ directory:
%   matlab -batch "generate_code"
%
% Output goes to matlab/codegen/ and is picked up by CMakeLists.txt.
% Re-run whenever detect_aruco_in_camera_frame.m changes.

pkg_root = fileparts(mfilename('fullpath'));

cfg = coder.config('lib');
cfg.TargetLang                        = 'C++';
cfg.GenerateReport                    = false;
cfg.SupportNonFinite                  = true;

% Example inputs — fixes image at 480×640 RGB (matches D435i 640p stream)
H   = 480; W = 640;
img = zeros(H, W, 3, 'uint8');
fx  = double(0); fy = double(0);
cx  = double(0); cy = double(0);
mm  = double(0);

orig_dir = pwd;
cd(pkg_root);
fprintf('Running MATLAB Coder — output: %s/codegen\n', pkg_root);
codegen detect_aruco_in_camera_frame -config cfg -args {img,fx,fy,cx,cy,mm} -d codegen
cd(orig_dir);
fprintf('Done.\n');
