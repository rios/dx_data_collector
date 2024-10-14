#ifndef _DX_ROS_MSG_H__
#define _DX_ROS_MSG_H__

#include "ros_msg_parser/ros_parser.hpp"
#include "ros_msg_parser/utils/span.hpp"
#include <topic_tools/shape_shifter.h>

#include <dx_rios_utils/base.h>

#include "data_types/field_data.h"

namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a message received from ROS
 * 
 */
class DxRosMsg
{
public:
  DECLARE_SMART_PTR(DxRosMsg)

public:
  /**
   * @brief Construct a new Dx Ros Msg object from a ShapeShifter msg
   * 
   * @param topic_name The name of the topic the message came from
   * @param msg The message
   * @param msg_parser The parser for this message
   * @param time_recvd The time the message was received, if available
   * 
   */
  DxRosMsg(const std::string& topic_name, 
           const RosMsgParser::ShapeShifter::ConstPtr msg,
           const RosMsgParser::Parser& msg_parser,
           std::optional<ros::Time> time_recvd = std::nullopt);

  /**
   * @brief Get access to the message data map
   * 
   * @return const std::map<std::string, FieldData>& The parsed message data 
   */
  const std::unordered_map<std::string, FieldData>& parsedMsgData();

  /**
   * @brief Get the time the message was received
   * 
   * @return const Ros::Time& The time the message was received
   */
  const ros::Time& timeRecvd() const;

  /**
   * @brief Get the time the message was sent. If there's no header, the time the message was received is used instead
   * 
   * @return const ros::Time& The time the message was sent
   */
  const ros::Time& msgTime();

  /**
   * @brief Set the time the message was received
   * 
   * @param time_recvd The time the message was received
   */
  void setTimeRecvd(const ros::Time& time_recvd);

  /**
   * @brief Get the name of the topic the message came from
   * 
   * @return const std::string& 
   */
  const std::string& topicName() const;

  /**
   * @brief Get a string representing the message type
   * 
   * @return const std::string& The message type
   */
  const std::string& msgType() const;

  /**
   * @brief The hash of the message
   * 
   * @return const std::string The message hash
   */
  const std::string& msgHash() const;

  /**
   * @brief Data span reresting message's raw data
   * 
   * @return const nonstd::span<const uint8_t> The data span
   */
  const nonstd::span<const uint8_t>& dataSpan() const;

  /**
   * @brief Return a shape shifter message based on thsi data
   * 
   * @param msg The shape shifter message
   */
  void toShapeShifter(topic_tools::ShapeShifter& msg) const;

private:
  /**
   * @brief Name of the topic the message came from
   * 
   */
  std::string topic_name_;

  /**
   * @brief Message md5
   * 
   */
  std::string md5_;

  /**
   * @brief The message parser
   * 
   */
  const RosMsgParser::Parser& msg_parser_;

  /**
   * @brief Pointer to the message coming from ROS - so it doesn't get deleted until we're done with it
   * 
   */
  const RosMsgParser::ShapeShifter::ConstPtr msg_;

  /**
   * @brief Pointer to binary message data
   * 
   */
  const uint8_t * data_;

  /**
   * @brief Raw binary data from message
   * 
   */
  nonstd::span<const uint8_t> data_span_;  

  /**
   * @brief The time the message was received
   * 
   */
  ros::Time time_recvd_;

  /**
   * @brief The time the message was sent
   * 
   */
  ros::Time msg_time_;

  /**
   * @brief Whether the message is parsed
   * 
   */
  bool is_parsed_ = false; 

  /**
   * @brief A map of all parsed message data
   * 
   */
  std::unordered_map<std::string, FieldData> parsed_msg_data_;
  
};

} /* data_collector */ } /* rios */

#endif /* _DX_ROS_MSG_H__ */