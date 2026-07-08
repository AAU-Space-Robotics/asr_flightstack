"""
Launch the ArUco detector test: aruco_sim feeds synthetic marker images to a
detector and checks the detections against ground truth.

  detector:=ref     pure-OpenCV aruco_detector_ref.py   (no MATLAB; default)
  detector:=matlab  the MATLAB-Coder aruco_detector      (needs generate_code.m)

Both detectors share the same topics/params, so only the executable changes.

Examples:
  ros2 launch asr_perception aruco_detector_test.launch.py
  ros2 launch asr_perception aruco_detector_test.launch.py detector:=matlab
  ros2 launch asr_perception aruco_detector_test.launch.py cycles:=0   # run forever
  ros2 launch asr_perception aruco_detector_test.launch.py visualize:=true cycles:=0
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node
from launch_ros.descriptions import ParameterValue

# Camera intrinsics — keep in sync with the aruco_detector defaults.
CAMERA = {
    'fx': 425.88,
    'fy': 425.88,
    'cx': 430.51,
    'cy': 238.53,
    'marker_size_mm': 150.0,
}


def generate_launch_description():
    detector = LaunchConfiguration('detector')
    # LaunchConfiguration resolves to a string; coerce to the param's real type
    # or rclpy rejects the override (int/bool param given a string).
    cycles = ParameterValue(LaunchConfiguration('cycles'), value_type=int)
    visualize = ParameterValue(LaunchConfiguration('visualize'), value_type=bool)
    use_ref = PythonExpression(["'", detector, "' == 'ref'"])

    sim = Node(
        package='asr_perception',
        executable='aruco_sim.py',
        name='aruco_sim',
        output='screen',
        parameters=[{
            **CAMERA,
            'publish_rate_hz': 5.0,
            'tvec_tol_mm':     50.0,
            'orient_tol_deg':  12.0,
            'cycles':          cycles,
            'visualize':       visualize,
        }],
    )

    detector_ref = Node(
        package='asr_perception',
        executable='aruco_detector_ref.py',
        name='aruco_detector_ref',
        output='screen',
        parameters=[CAMERA],
        condition=IfCondition(use_ref),
    )

    detector_matlab = Node(
        package='asr_perception',
        executable='aruco_detector',
        name='aruco_detector',
        output='screen',
        parameters=[CAMERA],
        condition=UnlessCondition(use_ref),
    )

    return LaunchDescription([
        DeclareLaunchArgument('detector', default_value='ref',
                              description="which detector: 'ref' or 'matlab'"),
        DeclareLaunchArgument('cycles', default_value='3',
                              description='scene cycles before summary (0 = forever)'),
        DeclareLaunchArgument('visualize', default_value='false',
                              description='show a live window with ground-truth overlay'),
        sim,
        detector_ref,
        detector_matlab,
    ])
