/**
 * @file ros_ingestion.cpp
 * @author Leo Keselman (leo.keselman@rios.ai)
 * @brief ROS ingestor implementation
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "ros_ingestion.h"

rios::data_collector::TopicIngestor::TopicIngestor(const rios::cfg& topic_config, RosDataBuffer& ros_data_buffer)
: nh_("~")
, ros_data_buffer_(ros_data_buffer)
{
  // Attempt to determine the config as just a string (indicating default settings for the topic)
  try
  {
    topic_name_ = topic_config.as<std::string>();
    throttle_period_s_ = NO_THROTTLE;
  }
  catch(const std::exception& e)
  {
    // the config is a map defining non-default settings
    if (!topic_config["topic"]) 
    {
      ROS_ERROR_STREAM("No 'topic' field defined in topics map. This is required. Topic will not be registered.");
      return;
    }
    
    topic_name_ = topic_config["topic"].as<std::string>();
    
    if (topic_config["throttle_period_s"])
    {
      throttle_period_s_ = topic_config["throttle_period_s"].as<float>();
    }
    else
    {
      throttle_period_s_ = NO_THROTTLE;
    }
  }

  // Subscribe to the topic
  topic_sub_ = nh_.subscribe(topic_name_, MSG_QUEUE_LENGTH, &rios::data_collector::TopicIngestor::topicCallback, this);
  
  ROS_INFO_STREAM("Successfully subscribed to " << topic_name_ << (throttle_period_s_ == NO_THROTTLE ? " without a throttle." : " with a throttle period of " + std::to_string(throttle_period_s_) + "s."));
}

void rios::data_collector::TopicIngestor::topicCallback(const ros::MessageEvent<RosMsgParser::ShapeShifter>& msg_event)
{
  const RosMsgParser::ShapeShifter::ConstPtr & msg = msg_event.getConstMessage();
  boost::shared_ptr<const ros::M_string> const& connection_header = msg_event.getConnectionHeaderPtr();

  // Find the sender in the connection header
  std::string sender = "unknown";
  if (connection_header)
  {
    ros::M_string::const_iterator it = connection_header->find("callerid");
    if(it != connection_header->end())
    {
      sender = it->second;
    }
  }

  bool latched_msg = false;
  if (connection_header)
  {
    ros::M_string::const_iterator it = connection_header->find("latching");
    if((it != connection_header->end()) && (it->second == "1"))
    {
      latched_msg = true;
    }
  }

  // Parser must be registered
  parsers_.registerParser(topic_name_, *msg);

  // Add the message to the buffer
  DxRosMsg::Ptr dx_msg = std::make_shared<DxRosMsg>(topic_name_, *msg, *parsers_.getParser(topic_name_));

  if (latched_msg)
  {
    ros_data_buffer_.addLatchedMsg(sender, dx_msg);
  }

  // Regardless - add to buffer in case latched message is constantly published (this may cause some duplicates in the output but it's ok)
  ros_data_buffer_.addMsg(dx_msg);
}

rios::data_collector::RosDataBuffer::RosDataBuffer(double max_buffer_time_s) 
: max_buffer_time_s_(max_buffer_time_s)
, nh_("~")
{
  buffer_clean_timer_ = nh_.createTimer(ros::Duration(CLEAN_PERIOD_S), 
    [this](const ros::TimerEvent&)
    {
      // Remove messages older than the max buffer time
      while (!data_queue_.empty())
      {
        if ((ros::Time::now() - data_queue_.front()->timeRecvd()).toSec() > max_buffer_time_s_)
        {
          data_queue_.pop_front();
        }
        else
        {
          break;
        }
      }
    }
  );
}

void rios::data_collector::RosDataBuffer::addMsg(DxRosMsg::Ptr msg)
{
  std::lock_guard guard(data_mutex_);

  // Add the message to the buffer
  data_queue_.push_back(msg);
}

void rios::data_collector::RosDataBuffer::addLatchedMsg(const std::string& sender_name, DxRosMsg::Ptr msg)
{
  std::lock_guard guard(data_mutex_);

  // Add the message to the buffer
  latched_msg_map_[sender_name] = msg;
}

bool rios::data_collector::RosDataBuffer::outputData(std::deque<DxRosMsg::Ptr>& out_queue, std::optional<ros::Time> start_time, std::optional<ros::Time> end_time)
{
  std::lock_guard guard(data_mutex_);

  ROS_INFO_STREAM("ROS Ingestor outputting " << data_queue_.size() << " regular messages and " << latched_msg_map_.size() << " latched messages.");

  /* We want to interleave latched messages and regular messages by their arrival time */

  // Grab the iterator for the start of the latched messages
  std::unordered_map<std::string, rios::data_collector::DxRosMsg::Ptr>::iterator latched_msg_it = latched_msg_map_.begin();

  // Add all non-latched messages in the window
  for (auto & message : data_queue_)
  {
    // Add all latched messages that arrived before this message
    while (latched_msg_it != latched_msg_map_.end() && latched_msg_it->second->timeRecvd() < message->timeRecvd())
    {
      // Set the time received to be right before the interlaced message
      latched_msg_it->second->setTimeRecvd(message->timeRecvd() - ros::Duration(0.0001));

      out_queue.push_back(latched_msg_it->second);
      latched_msg_it++;
    }

    // If we have no start time or the message time is after the start time, add it to the output queue
    if (!start_time || message->timeRecvd() >= start_time.value())
    {
      out_queue.push_back(message);
    }

    // If we have an end time and the message time is after the end time, stop
    if (end_time && message->timeRecvd() >= end_time.value())
    {
      break;
    }
  }

  return true;
}

rios::data_collector::RosDataIngestor::RosDataIngestor(const rios::cfg& ingestion_config, const std::string& episode_name)
: nh_("~")
, ingestion_config_(ingestion_config)
, ros_data_buffer_(ingestion_config["buffer_length_s"].as<double>())
, episode_name_(episode_name)
{
  // Create tf buffer
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>((ros::Duration)tf2::BufferCore::DEFAULT_CACHE_TIME, true);
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create topic ingestors
  for (auto & topic : ingestion_config["topics"])
  {
    topic_ingestors_.push_back(std::make_shared<TopicIngestor>(topic, ros_data_buffer_));
  }

  // Create the snapshot service server
  snapshot_service_ = nh_.advertiseService("snapshot", &RosDataIngestor::snapshotCallback, this);

  // Create the time service server
  get_time_service_ = nh_.advertiseService("get_time", &RosDataIngestor::getTime, this);
}

void rios::data_collector::RosDataIngestor::registerStoreCallback(std::function<void (std::deque<DxRosMsg::Ptr>, const std::string&)> callback)
{
  store_callbacks_.push_back(callback);
}

bool rios::data_collector::RosDataIngestor::snapshotCallback(dx_data_collector_msgs::Snapshot::Request& req, dx_data_collector_msgs::Snapshot::Response& res)
{
  // Grab the requested data from the buffer
  std::deque<DxRosMsg::Ptr> data_queue;
  std::optional<ros::Time> start_time = std::nullopt;
  std::optional<ros::Time> end_time = std::nullopt;
  if (!req.start_time.isZero()) start_time.emplace(req.start_time);
  if (!req.end_time.isZero()) end_time.emplace(req.end_time);

  ros_data_buffer_.outputData(data_queue, start_time, end_time);

  std::string snapshot_name = episode_name_;
  if (!req.snapshot_name.empty()) snapshot_name = req.snapshot_name;

  // Call the store callbacks
  for (auto & callback : store_callbacks_)
  {
    callback(data_queue, snapshot_name);
  }

  res.success = true;
  res.message = "Data storage triggered.";

  return true;
}

bool rios::data_collector::RosDataIngestor::getTime(dx_data_collector_msgs::GetTime::Request& req, dx_data_collector_msgs::GetTime::Response& res)
{
  res.cur_time = ros::Time::now();

  return true;
}

