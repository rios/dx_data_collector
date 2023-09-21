
import rospy
from dx_sysmon_interface.sysmon_sdk import sysmon_utils, sysmon_bases
import random


class SomeChecker(sysmon_bases.GenericChecker):

    @classmethod
    def key(cls):
        return "template_checker"

    def __init__(self, id_str, node_yaml):
        super().__init__(id_str, node_yaml)

    def check(self):
        condition_1 = random.randrange(
            0, 100) > self._settings['normal_percentage']
        condition_2 = random.randrange(
            0, 100) > self._settings['normal_percentage']
        if condition_1:
            message = "Condition 1 triggered"
            rospy.logwarn_throttle(60, message)
            return self._alarm.generate_trace(message)

        if condition_2:
            message = "condition 2 triggered"
            rospy.logwarn_throttle(60, message)
            return self._alarm.generate_trace(message)

        return sysmon_utils.Trace.ok()


class SomeOtherChecker(sysmon_bases.GenericChecker):

    @classmethod
    def key(cls):
        return "template_checker2"

    def __init__(self, id_str, node_yaml):
        super().__init__(id_str, node_yaml)

    def check(self):
        return sysmon_utils.Trace.ok()
