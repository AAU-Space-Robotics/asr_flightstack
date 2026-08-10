from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    gcs_pkg_share = FindPackageShare('asr_gcs')
    comms_path = PathJoinSubstitution([gcs_pkg_share, 'config', 'gcs_comms.yaml'])
            

    return LaunchDescription([
       

        # MAVLink bridge — GCS side, UDP loopback to comms_uav
        # Uses 14560/14561 to avoid clashing with QGroundControl (14550)
        Node(
            package='asr_comms',
            executable='comms_gcs',
            name='comms_gcs',
            namespace='asr/gcs',
            output='screen',
            parameters=[{
                'bind_port':   14560,
                'target_port': 14561,
            }],
        ),

        # Ground control station GUI
        Node(
            package='asr_gcs',
            executable='interface',
            name='aau_groundcontrol_node',
            namespace='asr/gcs',
            output='screen',
        ),

        Node(
            package='asr_drivers',
            executable='rtcm_reader.py',
            name='rtcm_reader',
            namespace='asr/gcs',
            output='screen',
            parameters=[comms_path],
        ),
        
    ])
