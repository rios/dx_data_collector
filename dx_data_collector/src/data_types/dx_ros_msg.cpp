/**
 * @file dx_ros_msg.cpp
 * @author Leo Keselman (leo.keselman@rios.ai)
 * @brief Implementation of DxRosMsg
 * @version 0.1
 * @date 2023-09-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "data_types/dx_ros_msg.h"

rios::data_collector::DxRosMsg::DxRosMsg(const std::string& topic_name, 
                                         const RosMsgParser::ShapeShifter::ConstPtr msg,
                                         const RosMsgParser::Parser& msg_parser,
                                         std::optional<ros::Time> time_recvd)
: topic_name_(topic_name)
, msg_parser_(msg_parser)
, msg_(msg)
{
  if (time_recvd) time_recvd_ = time_recvd.value();
  else time_recvd_ = ros::Time::now();

  data_ = msg->raw_data();

  // Create a span to access this data
  data_span_ = nonstd::span(data_, msg->size());

  // Store the md5 sum
  md5_ = msg->getMD5Sum();
}

const ros::Time& rios::data_collector::DxRosMsg::timeRecvd() const
{
  return time_recvd_;
}

const ros::Time& rios::data_collector::DxRosMsg::msgTime()
{
  if (parsedMsgData().count("/header/stamp"))
  {
    msg_time_ = std::get<ros::Time>(parsedMsgData().at("/header/stamp"));
  }
  else
  {
    msg_time_ = timeRecvd();
  }

  return msg_time_;
}

void rios::data_collector::DxRosMsg::setTimeRecvd(const ros::Time& time_recvd)
{
  time_recvd_ = time_recvd;
}

const std::string& rios::data_collector::DxRosMsg::topicName() const
{ 
  return topic_name_;
}

const std::string& rios::data_collector::DxRosMsg::msgType() const
{ 
  return msg_parser_.getMessageInfo()->type_list.front().type().baseName();
}

const std::string& rios::data_collector::DxRosMsg::msgHash() const
{
  return msg_parser_.getMessageMD5();
}

const nonstd::span<const uint8_t>& rios::data_collector::DxRosMsg::dataSpan() const
{
  return data_span_;
}

void rios::data_collector::DxRosMsg::toShapeShifter(topic_tools::ShapeShifter& msg) const
{
  msg.morph(msgHash(), msgType(), msg_parser_.getMessageDefinition(), "");

  // We need to grab a non-const pointer to the data so we need to copy the data from ths msg_ shared pointer
  std::vector<uint8_t> data_copy(data_span_.begin(), data_span_.end());

  ros::serialization::OStream stream(data_copy.data(), data_span_.size());
  msg.read(stream);
}

const std::unordered_map<std::string, rios::data_collector::FieldData>& rios::data_collector::DxRosMsg::parsedMsgData()
{
  // If already parsed, just return it
  if (is_parsed_) return parsed_msg_data_;

  msg_parser_.deserializeIntoDxRosMsg(data_span_, parsed_msg_data_);
  is_parsed_ = true;

  return parsed_msg_data_;
}
