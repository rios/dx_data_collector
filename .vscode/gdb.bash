#!/bin/sh
source /opt/rios/install/setup.bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/rios/install/lib
gdb $@