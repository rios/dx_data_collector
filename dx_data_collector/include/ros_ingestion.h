/**
 * @file ros_ingestion.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief ROS data ingestion
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __ROS_DATA_INGESTION_H__
#define __ROS_DATA_INGESTION_H__

#include <dx_data_collector_msgs/GetTime.h>
#include <dx_data_collector_msgs/Snapshot.h>
#include <dx_data_collector_msgs/SnapshotTrigger.h>
#include <dx_rios_utils/base.h>
#include <dx_rios_utils/json/rapidjson/prettywriter.h>
#include <dx_rios_yaml/yaml.h>
#include <ros/ros.h>
#include <std_msgs/Time.h>
#include <tf2/buffer_core.h>
#include <tf2_ros/transform_listener.h>

#include <chrono>
#include <string>
#include <unordered_map>

#include "data_types/dx_ros_msg.h"
#include "ros_msg_parser/ros_parser.hpp"

namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a buffer of messages
 * 
 */
class RosDataBuffer
{
public:
  DECLARE_SMART_PTR(RosDataBuffer)

  static constexpr float CLEAN_PERIOD_S = 0.100;

public:

  /**
   * @brief Construct a new Ros Data Buffer object
   * 
   * @param max_buffer_time_s The maximum buffer time in seconds
   * 
   */
  RosDataBuffer(double max_buffer_time_s);

  /**
   * @brief Add a message to the buffer
   * 
   * @param msg The message
   */
  void addMsg(DxRosMsg::Ptr msg);

  /**
   * @brief Add a latched message to the buffer
   * 
   * @param sender_name The sender name
   * @param msg The message
   */
  void addLatchedMsg(const std::string& sender_name, DxRosMsg::Ptr msg);

  /**
   * @brief Register a callback to call when the buffer is full. It will be called the first time the buffer is full and then subsequently when the whole buffer has been erased and is full with new data
   * 
   * @param callback The callback function
   */
  void registerBufferFullCallback(std::function<void ()> callback);

  /**
   * @brief Register a parameter to fetch from ROS on every output
   * 
   * @param param The parameter name
   */
  void registerParam(const std::string& param);

  /**
   * @brief Output the data in the buffer
   * 
   * @param out_queue The output queue
   * @param params The parameter values that were configured to be fetched from ROS
   * @param start_time The start time. Optional (defaults to start of buffer)
   * @param end_time The end time. Optional (defaults to end of buffer)
   * @return true Success
   * @return false Failure
   */
  bool outputData(std::deque<DxRosMsg::Ptr>& out_queue, std::unordered_map<std::string, std::shared_ptr<std::string>>& params, std::optional<ros::Time> start_time = std::nullopt, std::optional<ros::Time> end_time = std::nullopt);

private:

  /**
   * @brief Node handle
   * 
   */
  ros::NodeHandle nh_;

  /**
   * @brief A queue containing the current buffer of messages
   * 
   */
  std::deque<DxRosMsg::Ptr> data_queue_;

  /**
   * @brief Stores latched messages in list so one can be included in the output each time
   * 
   */
  std::unordered_map<std::string, DxRosMsg::Ptr> latched_msg_map_;

  /**
   * @brief The maximum buffer time in seconds
   * 
   */
  double max_buffer_time_s_;

  /**
   * @brief Mutex to access data buffer
   * 
   */
  std::mutex data_mutex_;

  /**
   * @brief Periodic task to clean the buffer
   * 
   */
  ros::Timer buffer_clean_timer_;

  /**
   * @brief Callbacks to call when the buffer fills up from last full
   * 
   */ 
  std::vector<std::function<void ()>> buffer_full_callbacks_;

  /**
   * @brief The last message in the queue when the last buffer full callback was called 
   * 
   */
  DxRosMsg::Ptr last_msg_in_queue_when_last_notified_;

  /**
   * @brief Whether to notify before the next time we delete a message
   * 
   */
  bool notify_next_delete_;

  /**
   * @brief Parameters to fetch from ROS
   * 
   */
  std::list<std::string> params_to_fetch_;

};

/**
 * @brief Encapsulates ingestion of a particular topic
 * 
 */
class TopicIngestor
{
public:
  DECLARE_SMART_PTR(TopicIngestor)

public:

  /**
   * @brief Construct the Topic Ingestor
   * 
   * @param topic_config The topic configuration
   * @param ros_data_buffer The buffer to add messages to when they are received
   */
  TopicIngestor(const rios::cfg& topic_config, RosDataBuffer& ros_data_buffer);

  /**
   * @brief Get the topic name
   *
   * @return const std::string& The topic name
   */
  const std::string & getTopicName() const { return topic_name_; }

  /**
   * @brief Returns true if timestamp filtering is enabled
   *
   * @return bool
   */
  bool timestampFilterEnabled() const { return logging_stamp_topic_.has_value(); }

  /**
   * @brief Returns true if this timestamp is in the set of relevant timestamps.
   *
   * @return bool
   */
  bool isTimeStampRelevant(const ros::Time & time) const
  {
    return relevant_timestamps_.find(time) != relevant_timestamps_.end();
  }

  /**
   * @brief Clear the relevant timestamps
   *
   */
  void clearRelevantTimestamps() { relevant_timestamps_.clear(); }

  /**
   * @brief Pause all ingestion
   * 
   */
  static void pause();

  /**
   * @brief Resume all ingestion
   * 
   */
  static void resume();

private:

  static constexpr float NO_THROTTLE = -1.0;
  static constexpr int MSG_QUEUE_LENGTH = 1000;
  static bool paused_;

  /**
   * @brief The nodehandle
   * 
   */
  ros::NodeHandle nh_;

  /**
   * @brief The topic subscriber
   * 
   */
  ros::Subscriber topic_sub_;

  /**
   * @brief The topic name
   * 
   */
  std::string topic_name_;

  /**
   * @brief The logging_stamp_topic subscriber
   *
   */
  std::optional<ros::Subscriber> logging_stamp_topic_sub_ = std::nullopt;

  /**
   * @brief Topic name to use for timestamp filtering
   *
   */
  std::optional<std::string> logging_stamp_topic_ = std::nullopt;

  /**
   * @brief Set of timestamps to use for filtering
   *
   */
  std::set<ros::Time> relevant_timestamps_;

  /**
   * @brief Throttle period for unchanging values
   * 
   */
  float throttle_period_s_ = NO_THROTTLE;

  /**
   * @brief Parsers collection
   * 
   */
  RosMsgParser::ParsersCollection parsers_;

  /**
   * @brief The buffer to add messages to when they are received
   * 
   */
  RosDataBuffer& ros_data_buffer_;

  /**
   * @brief Callback for the topic
   * 
   * @param msg_event The message event
   */
  void topicCallback(const ros::MessageEvent<RosMsgParser::ShapeShifter>& msg_event);

  /**
   * @brief Callback for the relevant timestamp topic
   *
   * @param msg
   */

  void relevantTimestampTopicCallback(const std_msgs::Time::ConstPtr & msg);
};

using IngestorMap = std::unordered_map<std::string, TopicIngestor::Ptr>;

/**
 * @brief Class to encapsulate data ingestion coming from ROS
 * 
 */
class RosDataIngestor
{
public:
  DECLARE_SMART_PTR(RosDataIngestor)

public:

  /**
   * @brief Construct the ROS Data Ingestor
   * 
   * @param ingestion_config The ingestion configuration
   * @param episode_name The name of the episode if any
   */
  RosDataIngestor(const rios::cfg& ingestion_config, const std::string& episode_name);

  /** 
   * @brief Register a callback to be called when the data should be stored
   * 
   * @param callback The callback function
  */
  void registerStoreCallback(std::function<void (std::deque<DxRosMsg::Ptr>, std::unordered_map<std::string, std::shared_ptr<std::string>>, const std::string&)> callback);

  /**
   * @brief Get the topic ingestors map
   *
   * @return const std::unordered_map<std::string, TopicIngestor::Ptr>& The topic ingestors map
   */
  const std::unordered_map<std::string, TopicIngestor::Ptr> & getTopicIngestors() const
  {
    return topic_ingestors_;
  }

private:

  class ScheduleEvent
  {
  public:
    DECLARE_SMART_PTR(ScheduleEvent)

  public:
    /**
     * @brief Construct a new Schedule Event object
     * 
     * @param schedule_config The config for the schedule event
     * @param action_callback The callback to call when the action is triggered
     */
    ScheduleEvent(const rios::cfg& schedule_config, std::function<void (const std::string&)> action_callback);

    /**
     * @brief Get the last time the action was triggered
     * 
     * @return std::chrono::system_clock::time_point The last time the action was triggered
     */
    std::chrono::system_clock::time_point getLastTime() const;

    /**
     * @brief Get the action
     * 
     * @return const std::string& The action
     */
    const std::string& getAction() const;

  private:
    const rios::cfg& schedule_config_;
    std::string action_;
    std::set<std::chrono::system_clock::time_point> schedule_times_;
    std::vector<std::function<void (const std::string&)>> action_callbacks_;
    ros::Timer schedule_timer_;
    ros::NodeHandle nh_;
  };

  ros::NodeHandle nh_;

  /**
   * @brief Ingestion configuration
   * 
   */
  const rios::cfg& ingestion_config_;

  /**
   * @brief Map of topic ingestors by topic
   * 
   */
  std::unordered_map<std::string, TopicIngestor::Ptr> topic_ingestors_;

  /**
   * @brief The ROS data buffer
   * 
   */
  RosDataBuffer ros_data_buffer_;

  /**
   * @brief Callbacks to call when the data should be stored
   * 
   */ 
  std::vector<std::function<void (std::deque<DxRosMsg::Ptr>, std::unordered_map<std::string, std::shared_ptr<std::string>>, const std::string&)>> store_callbacks_;

  /**
   * @brief Topic interface for requesting snapshot of the data
   *
   */
  ros::Subscriber snapshot_trigger_sub_;

  /**
   * @brief Service to take a snapshot of the data
   * 
   */ 
  ros::ServiceServer snapshot_service_;

  /**
   * @brief Service to get current time (used by debug gui)
   * 
   */
  ros::ServiceServer get_time_service_;

  /**
   * @brief Episode name
   * 
   */
  std::string episode_name_;

  /**
    * @brief 
    * 
    */
  std::shared_ptr <tf2_ros::Buffer> tf_buffer_;

  /**
    * @brief tf listener.
    * 
    */
  std::shared_ptr <tf2_ros::TransformListener> tf_listener_;

  /**
   * @brief Schedule events
   * 
   */
  std::vector<ScheduleEvent::Ptr> schedule_events_;

  /**
   * @brief Callback for the snapshot service
   * 
   * @param req The request
   * @param res The response
   * @return true Success
   * @return false Failure
   */
  bool snapshotCallback(dx_data_collector_msgs::Snapshot::Request& req, dx_data_collector_msgs::Snapshot::Response& res);

  /**
   * @brief Callback for the snapshot trigger topic interface
   *
   * @param req The trigger request
   */
  void snapshotTriggerCallback(const dx_data_collector_msgs::SnapshotTrigger::ConstPtr & req);

  /**
   * @brief Output data from the buffer
   * 
   * @param start_time Start time for the data. If not supplied, beginning of buffer
   * @param end_time End time for the data. If not supplied, end of buffer
   * @param snapshot_name The name of the snapshot
   * @return true Successfully output data
   * @return false Could not output data
   */
  bool outputData(
    std::optional<ros::Time> start_time = std::nullopt,
    std::optional<ros::Time> end_time = std::nullopt, const std::string & snapshot_name = "");

    /**
   * @brief Callback for the get_time service
   *
   * @param req The request
   * @param res The response
   * @return true Success
   * @return false Failure
   */
  bool getTime(dx_data_collector_msgs::GetTime::Request& req, dx_data_collector_msgs::GetTime::Response& res);

};


} /* data_collector */ } /* rios */


#endif /* __ROS_DATA_INGESTION_H__ */
