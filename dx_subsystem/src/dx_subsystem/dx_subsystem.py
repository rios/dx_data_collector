import rospy
from dx_sysmon_interface.sysmon_sdk import sysmon_bases, sysmon_implementations, sysmon_utils
from dx_subsystem.alarms import SomeChecker, SomeOtherChecker
from dx_subsystem_msgs.msg import SystemState
import random


class SubSystemNodeException(Exception):
    pass


class SubSystemNode:

    def __init__(self):

        self._config = rospy.get_param('~')
        self.namespace = self._config["namespace"]
        self.create_sysmon_alarms()
        self.state_publisher = rospy.Publisher(
            f"/subsystem/{self.namespace}/state", SystemState)
        self.run()

    def create_sysmon_alarms(self):
        rospy.loginfo("CREATING SYSMON ALARMS")
        some_checker_factory = sysmon_bases.Factory[SomeChecker]()
        some_other_checker_factory = sysmon_bases.Factory[SomeOtherChecker]()

        self.server = sysmon_implementations.AlarmsServer(sysmon_bases.Factory.chain(
            some_checker_factory, some_other_checker_factory))
        self.server.start()

    def run(self):
        r = rospy.Rate(self._config["publish_freq"])
        while not rospy.is_shutdown():
            state = random.choice(self._config["acceptable_states"])
            self.state_publisher.publish(state=state)
            r.sleep()
