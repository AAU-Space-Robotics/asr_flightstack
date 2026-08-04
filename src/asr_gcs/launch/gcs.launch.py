from launch import LaunchDescription
from launch.actions import Shutdown
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    gcs_pkg_share = FindPackageShare('asr_gcs')
    comms_path = PathJoinSubstitution([gcs_pkg_share, 'config', 'gcs_comms.yaml'])

    return LaunchDescription([
        # MAVLink bridge — GCS side
        Node(
            package='asr_comms',
            executable='comms_gcs',
            name='comms_gcs',
            namespace='asr/gcs',
            output='screen',
            parameters=[comms_path],
        ),

        # RTK base station — streams RTCM corrections to /rtcm
        Node(
            package='asr_drivers',
            executable='rtcm_reader.py',
            name='rtcm_reader',
            namespace='asr/gcs',
            output='screen',
            parameters=[comms_path],
        ),

        # GCS-side logger — records link_stats (radio/WiFi bandwidth and
        # SiK radio health for both ends) to a .ulg per GCS session
        Node(
            package='asr_logger',
            executable='logger_gcs',
            name='asr_logger_gcs',
            namespace='asr/gcs',
            output='screen',
            parameters=[comms_path],
        ),

        # Ground control station GUI
        Node(
            package='asr_gcs',
            executable='interface',
            name='aau_groundcontrol_node',
            namespace='asr/gcs',
            output='screen',
        ),
    ])
