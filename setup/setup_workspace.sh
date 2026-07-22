#!/bin/bash

# Get the absolute path of the script's directory (where the script is located)
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Set the ROS 2 workspace path and parent directory
ROS_WORKSPACE_PATH=$(dirname "$SCRIPT_DIR")  # Base workspace is drone-software (parent of setup/)
SRC_DIR="$ROS_WORKSPACE_PATH/src"
PARENT_DIR=$(dirname "$ROS_WORKSPACE_PATH")  # Directory containing drone-software
ROS_DISTRO="jazzy"

# 1. Install additional ROS dependencies
echo "Installing additional ROS $ROS_DISTRO dependencies..."
if ! dpkg -l | grep -q "ros-$ROS_DISTRO-serial-driver"; then
    if ! sudo apt-get install -y "ros-$ROS_DISTRO-serial-driver"; then
        echo "Error: Failed to install ros-$ROS_DISTRO-serial-driver"
        exit 1
    fi
else
    echo "ros-$ROS_DISTRO-serial-driver is already installed."
fi

if ! dpkg -l | grep -q "ros-$ROS_DISTRO-asio-cmake-module"; then
    if ! sudo apt-get install -y "ros-$ROS_DISTRO-asio-cmake-module"; then
        echo "Error: Failed to install ros-$ROS_DISTRO-asio-cmake-module"
        exit 1
    fi
else
    echo "ros-$ROS_DISTRO-asio-cmake-module is already installed."
fi

# 2. Check if the src directory exists, create it if it doesn't
if [ ! -d "$SRC_DIR" ]; then
  echo "Creating src directory at $SRC_DIR..."
  mkdir -p "$SRC_DIR"
fi

# 3. Navigate to the src directory
cd "$SRC_DIR" || exit

# 4. Function to clone a repository if it doesn't already exist
clone_repo_if_not_exists() {
    local repo_url="$1"
    local target_dir="$2"
    local branch="$3"
    if [ ! -d "$target_dir" ]; then
        echo "Cloning $repo_url into $target_dir..."
        if [ -n "$branch" ]; then
            git clone -b "$branch" "$repo_url" "$target_dir"
        else
            git clone "$repo_url" "$target_dir"
        fi
    else
        echo "$target_dir already exists, skipping clone."
    fi
}

# 5. Clone the asr_px4_msgs repository
mkdir -p deps
clone_repo_if_not_exists "git@github.com:AAU-Space-Robotics/asr_px4_msgs.git" "deps/px4_msgs"

# 6. Navigate to the parent directory to check/install Micro-XRCE-DDS-Agent and asr_PX4-Autopilot
cd "$PARENT_DIR" || exit

# 7. Check and install Micro-XRCE-DDS-Agent
if [ -d "Micro-XRCE-DDS-Agent" ]; then
  echo "Micro-XRCE-DDS-Agent is already installed, skipping installation."
else
  echo "Installing Micro-XRCE-DDS-Agent..."
  git clone https://github.com/eProsima/Micro-XRCE-DDS-Agent.git
  cd Micro-XRCE-DDS-Agent
  mkdir build
  cd build
  cmake ..
  make
  sudo make install
  sudo ldconfig /usr/local/lib/
  cd "$PARENT_DIR" || exit
fi        

# 8. Check and install asr_PX4-Autopilot
if [ -d "asr_PX4" ]; then
  echo "asr_PX4 is already installed, skipping installation."
else
  echo "Installing asr_PX4..."
  git clone git@github.com:AAU-Space-Robotics/asr_PX4.git --recursive
  cd asr_PX4
  bash ./Tools/setup/ubuntu.sh
  make px4_sitl
  cd "$PARENT_DIR" || exit
fi

echo "Workspace setup completed! Ready for build. Please build it now to use it :)"
