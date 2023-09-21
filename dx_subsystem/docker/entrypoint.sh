#!/bin/bash
set -e

# setup  environment
source "/opt/ros/$ROS_DISTRO/setup.bash"
source "/opt/rios/install/setup.bash"

exec "$@"
