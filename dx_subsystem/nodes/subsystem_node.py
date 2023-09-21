#!/usr/bin/env python3

import rospy
from dx_subsystem.dx_subsystem import SubSystemNode, SubSystemNodeException

if __name__ == '__main__':
    try:
        # most likely this name is overriden by roslaunch
        rospy.init_node('subsystem_node', log_level=rospy.INFO)
        node = SubSystemNode()
        rospy.spin()
    except SubSystemNodeException as e:
        rospy.logfatal('{}: Shutting down template node'.format(e))
    except rospy.ROSInterruptException:
        pass
