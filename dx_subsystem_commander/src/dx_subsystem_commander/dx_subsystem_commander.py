from dx_subsystem_msgs.msg import SystemState
from dx_subsystem_msgs.srv import SystemService
import rospy


QUERY_RATE_HZ = 10


class SubsystemInterface:
    """Class to interact with your subsystem from Python
    """

    def __init__(self, namespace: str, timeout_seconds: float = 10):
        """Create and IO point controller object

        Args:
            namespace (str): Always a good idea to namespace things
            timeout_seconds (float, optional): Always a good idea to have a timeout

        Raises:
            TimeoutError: The system did not connect withing timeout
        """

        # Read the I/O configuration to set up subscribers and publishers for this I/O
        self.namespace = namespace

        query_start_time = rospy.Time.now()
        r = rospy.Rate(QUERY_RATE_HZ)
        while (rospy.Time.now() - query_start_time).to_sec() < timeout_seconds and not (system_available := self.__check_system_available()):
            r.sleep()
        if not system_available:
            raise TimeoutError

        self.system_state_sub = rospy.Subscriber(
            f"/subsystem/{self.namespace}/state", SystemState, self.state_cb, queue_size=1)

        self.latest_state = None

    def __check_system_available(self):
        """Usually a good idea to check the system is available
        """
        return True

    def get_latest_state(self):
        """And now utility functions"""
        return self.latest_state

    def state_cb(self, msg):
        self.latest_state = msg.state
