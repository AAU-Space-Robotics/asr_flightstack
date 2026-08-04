from launch import LaunchDescription
from launch.actions import RegisterEventHandler, Shutdown
from launch.event_handlers import OnProcessExit
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    gcs_pkg_share = FindPackageShare('asr_gcs')
    comms_path = PathJoinSubstitution([gcs_pkg_share, 'config', 'gcs_comms.yaml'])

    # Ground control station GUI
    interface_node = Node(
        package='asr_gcs',
        executable='interface',
        name='aau_groundcontrol_node',
        namespace='asr/gcs',
        output='screen',
        additional_env={
            'LIBGL_ALWAYS_SOFTWARE': '1',
            'WAYLAND_DISPLAY': '',   # force GLFW onto X11/GLX so PyOpenGL can detect the context
        },
    )

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

        interface_node,

        # Shut the whole launch down when the GUI exits (closed or crashed) -
        # comms_gcs/rtcm_reader/logger_gcs would otherwise keep running with
        # nothing left to stop them, especially with no terminal attached.
        RegisterEventHandler(
            OnProcessExit(
                target_action=interface_node,
                on_exit=[Shutdown(reason='GCS GUI exited')],
            )
        ),
    ])
