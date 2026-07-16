#!/bin/bash

# Runs the ASR GCS (ros2 launch asr_gcs gcs.launch.py) with the workspace
# environment and Python venv. Used by the desktop launchers created by
# setup_gcs.sh, but can also be run directly from a terminal.
#
# With a terminal attached, output stays in the terminal. Without one
# (desktop "app" mode), output is written to ~/.local/state/asr_gcs/gcs.log.

SCRIPT_DIR=$(dirname "$(realpath "$0")")
WORKSPACE_DIR=$(dirname "$SCRIPT_DIR")
ROS_DISTRO="jazzy"
LOG_FILE="$HOME/.local/state/asr_gcs/gcs.log"

fail() {
    echo "Error: $1" >&2
    if [ -t 0 ]; then
        read -rp "Press Enter to close..."
    elif command -v zenity >/dev/null 2>&1; then
        zenity --error --title="ASR GCS" --text="$1" 2>/dev/null
    elif command -v notify-send >/dev/null 2>&1; then
        notify-send -u critical "ASR GCS" "$1"
    fi
    exit 1
}

[ -f "/opt/ros/$ROS_DISTRO/setup.bash" ] || fail "ROS 2 $ROS_DISTRO not found at /opt/ros/$ROS_DISTRO."
source "/opt/ros/$ROS_DISTRO/setup.bash"

[ -f "$WORKSPACE_DIR/install/setup.bash" ] || fail "Workspace not built. Run 'colcon build' in $WORKSPACE_DIR first."
source "$WORKSPACE_DIR/install/setup.bash"

# Activate the venv last so its python is first in PATH; ROS packages stay
# available through the PYTHONPATH set by the overlays above.
[ -f "$WORKSPACE_DIR/.venv/bin/activate" ] || fail "Python venv missing. Run setup/setup_venv.sh first."
source "$WORKSPACE_DIR/.venv/bin/activate"

if [ -t 1 ]; then
    exec ros2 launch asr_gcs gcs.launch.py
else
    mkdir -p "$(dirname "$LOG_FILE")"
    exec ros2 launch asr_gcs gcs.launch.py > "$LOG_FILE" 2>&1
fi
