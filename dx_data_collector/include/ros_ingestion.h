/**
 * @file ros_ingestion.h
 * @author leo keselman (leo.keselman@rios.ai)
 * @brief ROS data ingestion
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __ROS_DATA_INGESTION_H__
#define __ROS_DATA_INGESTION_H__


#include <ros/ros.h>
#include <tf2/buffer_core.h>
#include <tf2_ros/transform_listener.h>

#include <dx_rios_utils/base.h>
#include <dx_rios_utils/json/rapidjson/prettywriter.h>
#include <dx_rios_yaml/yaml.h>
#include <dx_data_collector_msgs/Snapshot.h>
#include <dx_data_collector_msgs/GetTime.h>

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
   * @brief Output the data in the buffer
   * 
   * @param out_queue The output queue
   * @param start_time The start time. Optional (defaults to start of buffer)
   * @param end_time The end time. Optional (defaults to end of buffer)
   * @return true Success
   * @return false Failure
   */
  bool outputData(std::deque<DxRosMsg::Ptr>& out_queue, std::optional<ros::Time> start_time = std::nullopt, std::optional<ros::Time> end_time = std::nullopt);

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

private:

  static constexpr float NO_THROTTLE = -1.0;
  static constexpr int MSG_QUEUE_LENGTH = 1000;

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
   * @brief Throttle period for unchanging values
   * 
   */
  float throttle_period_s_;

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
   * @param msg The message
   */
  void topicCallback(const RosMsgParser::ShapeShifter& msg);

};

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
  void registerStoreCallback(std::function<void (std::deque<DxRosMsg::Ptr>, const std::string&)> callback);


private:

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
  std::vector<TopicIngestor::Ptr> topic_ingestors_;

  /**
   * @brief The ROS data buffer
   * 
   */
  RosDataBuffer ros_data_buffer_;

  /**
   * @brief Callbacks to call when the data should be stored
   * 
   */ 
  std::vector<std::function<void (std::deque<DxRosMsg::Ptr>, const std::string&)>> store_callbacks_;

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
   * @brief Callback for the snapshot service
   * 
   * @param req The request
   * @param res The response
   * @return true Success
   * @return false Failure
   */
  bool snapshotCallback(dx_data_collector_msgs::Snapshot::Request& req, dx_data_collector_msgs::Snapshot::Response& res);

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
