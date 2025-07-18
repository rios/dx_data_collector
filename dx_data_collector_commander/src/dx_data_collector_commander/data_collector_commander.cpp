#include "data_collector_commander.h"

rios::data_collection::DataCollectorCommander::DataCollectorCommander(const std::string & collector_id)
: nh_("~")
, collector_id_(collector_id)
{
  snapshot_service_ = nh_.serviceClient<dx_data_collector_msgs::Snapshot>("/" + collector_id + "/snapshot");

  snapshot_trigger_pub_ =
    nh_.advertise<dx_data_collector_msgs::SnapshotTrigger>("snapshot_trigger", 5);

  record_start_time_.reset();
}

bool rios::data_collection::DataCollectorCommander::takeSnapshot(std::optional<double> time_s, std::optional<const std::string> snapshot_name)
{
  if (!snapshot_service_.exists())
  {
    ROS_WARN_STREAM("Attempt to take snapshot from collector " << collector_id_ << " which isn't available.");
    return false;
  }

  dx_data_collector_msgs::SnapshotRequest request;
  
  if (time_s)
  {
    // Set a specific time for snapshot
    request.end_time = ros::Time::now();
    request.start_time = ros::Time::now() - ros::Duration(time_s.value());
  }
  else
  {
    // Capture all the data in the buffer  - using start and end time of 0
    request.end_time = request.start_time = ros::Time(0);
  }

  if (snapshot_name)
  {
    // Specific snapshot name
    request.snapshot_name = snapshot_name.value();
  }
  else
  {
    // Name comes from episode name
    request.snapshot_name = "";
  }

  dx_data_collector_msgs::SnapshotResponse response;
  if (!snapshot_service_.call(request, response))
  {
    ROS_WARN_STREAM("Could not request snapshot. ROS_IP or message version mismatch.");
    return false;
  }

  if (!response.success)
  {
    ROS_WARN_STREAM("Could not request snapshot: " << response.message);
    return false;
  }

  return true;
}

void rios::data_collection::DataCollectorCommander::takeSnapshotFF(
  std::optional<double> time_s, std::optional<const std::string> snapshot_name)
{
  dx_data_collector_msgs::SnapshotTrigger request;

  if (time_s) {
    // Set a specific time for snapshot
    request.end_time = ros::Time::now();
    request.start_time = ros::Time::now() - ros::Duration(time_s.value());
  } else {
    // Capture all the data in the buffer  - using start and end time of 0
    request.end_time = request.start_time = ros::Time(0);
  }

  if (snapshot_name) {
    // Specific snapshot name
    request.snapshot_name = snapshot_name.value();
  } else {
    // Name comes from episode name
    request.snapshot_name = "";
  }

  snapshot_trigger_pub_.publish(request);
}

bool rios::data_collection::DataCollectorCommander::startRecording()
{
  if (record_start_time_)
  {
    ROS_WARN_STREAM("Restarting snapshot recording over an existing recording that wasn't stopped.");
  }

  record_start_time_ = ros::Time::now();

  return true;
}

bool rios::data_collection::DataCollectorCommander::stopRecording(std::optional<const std::string> snapshot_name)
{
  if (!record_start_time_)
  {
    ROS_WARN_STREAM("No recording was started - cannot stop recording.");
    return false;
  }

  double snapshot_duration = (ros::Time::now() - record_start_time_.value()).toSec();
  return takeSnapshot(snapshot_duration, snapshot_name);
}

bool rios::data_collection::DataCollectorCommander::stopRecordingFF(
  std::optional<const std::string> snapshot_name)
{
  if (!record_start_time_) {
    ROS_WARN_STREAM("No recording was started - cannot stop recording.");
    return false;
  }

  double snapshot_duration = (ros::Time::now() - record_start_time_.value()).toSec();
  takeSnapshotFF(snapshot_duration, snapshot_name);
  return true;
}
