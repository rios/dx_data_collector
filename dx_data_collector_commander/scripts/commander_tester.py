#!/opt/mamba/envs/rios/bin/python

from dx_data_collector_commander.data_collector_commander import DataCollectorCommander

from dx_rios_pybind import ros
import rospy

ros.init_node("data_commander_tester")

rospy.sleep(.5)

data_collector = DataCollectorCommander()

success = data_collector.take_snapshot()
rospy.loginfo(f"Snapshot result: {success}")

success = data_collector.stop_recording() # Should fail with a warning
rospy.loginfo(f"Stop recording before start recording: {success}")

# Record 2 seconds of data and output
data_collector.start_recording()
rospy.sleep(2.0)
success = data_collector.stop_recording()
rospy.loginfo(f"Stop recording result: {success}")

rospy.sleep(5.0)
# Take a snapshot of the last 5s and give it a name
success = data_collector.take_snapshot(5.0, "test_snapshot")
rospy.loginfo(f"5s snapshot result: {success}")
