#!/bin/bash
set -e

# setup  environment
source "/opt/ros/$ROS_DISTRO/setup.bash"
source "/opt/rios/install/setup.bash"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/rios/install/lib

exec "$@"
