"""Brings up the full mission-testing stack against thyra's PX4/Gazebo sim.

Disposable dev/test launch file -- lives outside thyra and asr_gcs so it
never conflicts with either. Delete once asr_gcs integrates directly and
this kind of manual sim testing is no longer needed.

Includes thyra_sim.launch.py (PX4 SITL + Gazebo, MicroXRCEAgent, comms_uav,
asr_autopilot -- all namespaced asr/thyra) and adds the two pieces it
doesn't start: mission_executor (same namespace, so its UAVCommand action
client and mission topics line up with comms_uav/asr_autopilot) and
comms_gcs (no namespace -- stands in for a separate GCS machine, with ports
crossed to match comms_uav's launch-file overrides: comms_uav binds 14561/
sends to 14560, so comms_gcs binds 14560/sends to 14561).

Usage:
    ros2 launch src/asr_mission/tools/sim_mission_system.launch.py

Once it's up (give PX4/Gazebo ~20-30s to boot), arm before running a plan --
arming is deliberately not a plan skill, see asr_mission/README.md:
    ros2 action send_goal /asr/thyra/in/uav_command \\
        asr_comms/action/UAVCommand "{command_type: 'arm'}"

Then drive a plan through it with:
    ros2 run asr_mission mission_cli <plan_file.json> [--vehicle thyra] [--abort-after N]
"""
import os

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    thyra_sim_launch = PathJoinSubstitution(
        [FindPackageShare('thyra'), 'launch', 'sim', 'thyra_sim.launch.py']
    )

    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(thyra_sim_launch)
        ),

        # Onboard mission executor -- same namespace as comms_uav/asr_autopilot
        # so in/mission_upload, in/mission_start, out/mission_validate,
        # out/mission_status, and the UAVCommand action client all resolve
        # to /asr/thyra/... and line up with what comms_uav bridges.
        Node(
            package='asr_mission',
            executable='mission_executor',
            name='mission_executor',
            namespace='asr/thyra',
            output='screen',
            parameters=[{'vehicle': 'thyra'}],
        ),

        # Stands in for a separate GCS machine -- no namespace, ports crossed
        # against comms_uav's overrides (14561 bind / 14560 target) so the
        # two only ever talk over the real WiFi/mavlink bridge, never a
        # same-process ROS topic shortcut.
        Node(
            package='asr_comms',
            executable='comms_gcs',
            name='comms_gcs',
            output='screen',
            parameters=[{
                'bind_port': 14560,
                'target_port': 14561,
            }],
        ),
    ])