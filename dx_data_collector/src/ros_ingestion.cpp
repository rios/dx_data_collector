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

bool rios::data_collector::TopicIngestor::paused_ = false;

void rios::data_collector::TopicIngestor::pause()
{
  paused_ = true;
}

void rios::data_collector::TopicIngestor::resume()
{
  paused_ = false;
}

rios::data_collector::TopicIngestor::TopicIngestor(
  const rios::cfg & topic_config, RosDataBuffer & ros_data_buffer)
: nh_("~"), ros_data_buffer_(ros_data_buffer)
{
  // Attempt to determine the config as just a string (indicating default settings for the topic)
  try {
    topic_name_ = topic_config.as<std::string>();
    throttle_period_s_ = NO_THROTTLE;
  } catch (const std::exception & e) {
    // the config is a map defining non-default settings
    if (!topic_config["topic"]) {
      ROS_ERROR_STREAM(
        "No 'topic' field defined in topics map. This is required. Topic will not be registered.");
      return;
    }

    topic_name_ = topic_config["topic"].as<std::string>();

    if (topic_config["logging_stamp_topic"]) {
      const std::string ts_topic_str = topic_config["logging_stamp_topic"].as<std::string>();
      if (ts_topic_str != "") {
        logging_stamp_topic_ = ts_topic_str;
      } else {
        ROS_ERROR_STREAM(
          "Empty logging_stamp_topic string. Timestamp filtering will not be enabled.");
      }
    } else if (topic_config["throttle_period_s"]) {
      throttle_period_s_ = topic_config["throttle_period_s"].as<float>();
    } else {
      throttle_period_s_ = NO_THROTTLE;
    }
  }

  // Subscribe to the topic
  topic_sub_ = nh_.subscribe(
    topic_name_, MSG_QUEUE_LENGTH, &rios::data_collector::TopicIngestor::topicCallback, this);

  ROS_INFO_STREAM(
    "Successfully subscribed to " << topic_name_
                                  << (throttle_period_s_ == NO_THROTTLE
                                        ? " without a throttle."
                                        : " with a throttle period of " +
                                            std::to_string(throttle_period_s_) + "s."));

  if (logging_stamp_topic_) {
    logging_stamp_topic_sub_ = nh_.subscribe(
      *logging_stamp_topic_, MSG_QUEUE_LENGTH,
      &rios::data_collector::TopicIngestor::relevantTimestampTopicCallback, this);

    ROS_INFO_STREAM(
      "..also successfully subscribed to associated logging_stamp_topic " << *logging_stamp_topic_);
  }
}

void rios::data_collector::TopicIngestor::topicCallback(const ros::MessageEvent<RosMsgParser::ShapeShifter>& msg_event)
{
  // Don't record if we're paused
  if (paused_) return;

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
  DxRosMsg::Ptr dx_msg = std::make_shared<DxRosMsg>(topic_name_, msg, *parsers_.getParser(topic_name_));

  if (latched_msg)
  {
    ros_data_buffer_.addLatchedMsg(sender, dx_msg);
  }

  // Regardless - add to buffer in case latched message is constantly published (this may cause some duplicates in the output but it's ok)
  ros_data_buffer_.addMsg(dx_msg);
}

void rios::data_collector::TopicIngestor::relevantTimestampTopicCallback(
  const std_msgs::Time::ConstPtr & msg)
{
  // Don't record if we're paused
  if (paused_) return;

  relevant_timestamps_.insert(msg->data);
}

rios::data_collector::RosDataBuffer::RosDataBuffer(double max_buffer_time_s) 
: max_buffer_time_s_(max_buffer_time_s)
, nh_("~")
{
  last_msg_in_queue_when_last_notified_ = nullptr;
  notify_next_delete_ = false;

  buffer_clean_timer_ = nh_.createTimer(ros::Duration(CLEAN_PERIOD_S), 
    [this](const ros::TimerEvent&)
    {
      // Remove messages older than the max buffer time
      while (!data_queue_.empty())
      {
        if ((ros::Time::now() - data_queue_.front()->timeRecvd()).toSec() > max_buffer_time_s_)
        {
          // About to remove messages - check if we need to notify anyone
          if (buffer_full_callbacks_.size() > 0)
          {
            // At least one callback wants to know
            if (last_msg_in_queue_when_last_notified_ == nullptr || notify_next_delete_)
            {
              notify_next_delete_ = false;

              // First time buffer is full
              last_msg_in_queue_when_last_notified_ = data_queue_.back();
              // Notify all callbacks
              for (auto & callback : buffer_full_callbacks_)
              {
                callback();
              }
            }
            else if (data_queue_.front() == last_msg_in_queue_when_last_notified_)
            {
              // We're about to remove the last message that was in the queue when we last notified
              notify_next_delete_ = true;
            }
          }

          // Remove the message
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

void rios::data_collector::RosDataBuffer::registerBufferFullCallback(std::function<void ()> callback)
{
  buffer_full_callbacks_.push_back(callback);
}

void rios::data_collector::RosDataBuffer::registerParam(const std::string& param)
{
  params_to_fetch_.push_back(param);
  ROS_INFO_STREAM("Successfully registered parameter " << param << " for fetching at output");
}

bool rios::data_collector::RosDataBuffer::outputData(std::deque<DxRosMsg::Ptr>& out_queue, std::unordered_map<std::string, std::shared_ptr<std::string>>& params, std::optional<ros::Time> start_time, std::optional<ros::Time> end_time)
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

  // Grab any requested ROS params and add them to the queue
  for (auto & param : params_to_fetch_)
  {
    params[param] = std::make_shared<std::string>();
    if (!nh_.getParamCached(param, *params[param]))
    {
      ROS_WARN_STREAM("Could not fetch parameter " << param << ". Current only scalar params are supported (int, float, bool, string)");
    }
  }

  return true;
}

rios::data_collector::RosDataIngestor::RosDataIngestor(
  const rios::cfg & ingestion_config, const std::string & episode_name)
: nh_("~"),
  ingestion_config_(ingestion_config),
  ros_data_buffer_(ingestion_config["buffer_length_s"].as<double>()),
  episode_name_(episode_name)
{
  // If we're in continuous mode, register a callback everytime the buffer fills up
  if (ingestion_config["continuous_mode"] && ingestion_config["continuous_mode"].as<bool>()) {
    ros_data_buffer_.registerBufferFullCallback([this]() { outputData(); });
  }

  // Create tf buffer
  tf_buffer_ =
    std::make_shared<tf2_ros::Buffer>((ros::Duration)tf2::BufferCore::DEFAULT_CACHE_TIME, true);
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create topic ingestors
  for (auto & topic : ingestion_config["topics"]) {
    std::shared_ptr<TopicIngestor> ingestor =
      std::make_shared<TopicIngestor>(topic, ros_data_buffer_);
    topic_ingestors_[ingestor->getTopicName()] = ingestor;
  }

  // Register parameters to collect
  if (ingestion_config["params"]) {
    for (auto & param : ingestion_config["params"]) {
      ros_data_buffer_.registerParam(param.as<std::string>());
    }
  }

  // Create the snapshot service server
  snapshot_service_ = nh_.advertiseService("snapshot", &RosDataIngestor::snapshotCallback, this);

  // Create the time service server
  get_time_service_ = nh_.advertiseService("get_time", &RosDataIngestor::getTime, this);

  // Check if working hours are defined and if so, create schedule events to pause/resume data collection at the appropriate times
  if (ingestion_config["schedule"]) {
    for (auto & schedule_config : ingestion_config["schedule"]) {
      schedule_events_.push_back(
        std::make_shared<ScheduleEvent>(
          schedule_config.as<rios::cfg>(), [this](const std::string & action) {
            if (action == "pause") {
              ROS_INFO_STREAM("Pausing data collection according to schedule.");
              TopicIngestor::pause();
            } else if (action == "resume") {
              ROS_INFO_STREAM("Resuming data collection according to schedule.");
              TopicIngestor::resume();
            } else {
              ROS_ERROR_STREAM("Invalid action in schedule config: " << action);
            }
          }));
    }

    std::chrono::system_clock::time_point last_time = (*schedule_events_.begin())->getLastTime();
    std::string last_action = (*schedule_events_.begin())->getAction();
    for (auto & event : schedule_events_) {
      if (event->getLastTime() > last_time) {
        last_time = event->getLastTime();
        last_action = event->getAction();
      }
    }

    if (last_action == "pause") {
      ROS_INFO_STREAM("Pausing data collection as it was the last scheduled event.");
      TopicIngestor::pause();
    } else if (last_action == "resume") {
      ROS_INFO_STREAM("Resuming data collection as it was the last scheduled event.");
      TopicIngestor::resume();
    }
  }
}

void rios::data_collector::RosDataIngestor::registerStoreCallback(std::function<void (std::deque<DxRosMsg::Ptr>, std::unordered_map<std::string, std::shared_ptr<std::string>>, const std::string&)> callback)
{
  store_callbacks_.push_back(callback);
}

bool rios::data_collector::RosDataIngestor::snapshotCallback(dx_data_collector_msgs::Snapshot::Request& req, dx_data_collector_msgs::Snapshot::Response& res)
{
  
  std::optional<ros::Time> start_time = std::nullopt;
  std::optional<ros::Time> end_time = std::nullopt;
  if (!req.start_time.isZero()) start_time.emplace(req.start_time);
  if (!req.end_time.isZero()) end_time.emplace(req.end_time);

  std::string snapshot_name = episode_name_;
  if (!req.snapshot_name.empty()) snapshot_name = req.snapshot_name;

  res.success = outputData(start_time, end_time, snapshot_name);
  res.message = "Data storage triggered.";

  return res.success;
}

bool rios::data_collector::RosDataIngestor::outputData(std::optional<ros::Time> start_time , std::optional<ros::Time> end_time, const std::string& snapshot_name)
{
  // Grab the requested data from the buffer
  std::deque<DxRosMsg::Ptr> data_queue;
  std::unordered_map<std::string, std::shared_ptr<std::string>> params;

  ros_data_buffer_.outputData(data_queue, params, start_time, end_time);

  // Call the store callbacks
  for (auto & callback : store_callbacks_)
  {
    callback(data_queue, params, snapshot_name);
  }

  return true;
}

bool rios::data_collector::RosDataIngestor::getTime(dx_data_collector_msgs::GetTime::Request& req, dx_data_collector_msgs::GetTime::Response& res)
{
  res.cur_time = ros::Time::now();

  return true;
}

rios::data_collector::RosDataIngestor::ScheduleEvent::ScheduleEvent(const rios::cfg& schedule_config, std::function<void (const std::string&)> action_callback)
: schedule_config_(schedule_config)
, nh_("~")
{
  action_callbacks_.push_back(action_callback);
  action_ = schedule_config["action"].as<std::string>();

  // Go through each day of the week and convert the time to a chrono time point
  for (auto& day: schedule_config["days"])
  {
    std::string day_str = day.as<std::string>();
    std::string time_str = schedule_config["time"].as<std::string>();

    // Parse the time string
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%H:%M:%S");

    // Find the next timepoint for this time including day of the week
    // Get the current day of the week
    std::time_t now = std::time(nullptr);
    std::tm* today_start = std::localtime(&now);
    today_start->tm_hour = 0;
    today_start->tm_min = 0;
    today_start->tm_sec = 0;
    int cur_day = today_start->tm_wday;

    // Find the next day of the week that matches the day string
    int schedule_day;
    if (day_str == "Sun") schedule_day = 0;
    else if (day_str == "Mon") schedule_day = 1;
    else if (day_str == "Tue") schedule_day = 2;
    else if (day_str == "Wed") schedule_day = 3;
    else if (day_str == "Thu") schedule_day = 4;
    else if (day_str == "Fri") schedule_day = 5;
    else if (day_str == "Sat") schedule_day = 6;
    else
    {
      ROS_ERROR_STREAM("Invalid day string in schedule config: " << day_str);
      continue;
    }

    // Find the next day of the week that matches the day string
    int days_to_add = schedule_day - cur_day;
    if (days_to_add < 0) days_to_add += 7;

    std::time_t today_start_time = std::mktime(today_start);
    std::chrono::system_clock::time_point today_start_chrono = std::chrono::system_clock::from_time_t(today_start_time);
    std::chrono::system_clock::time_point next_day = today_start_chrono + std::chrono::hours(24 * days_to_add);
    std::chrono::system_clock::time_point next_time = next_day + std::chrono::hours(tm.tm_hour) + std::chrono::minutes(tm.tm_min) + std::chrono::seconds(tm.tm_sec);
    if (next_time < std::chrono::system_clock::now()) next_time += std::chrono::hours(24*7);

    // Store the next timepoint in chronological order
    schedule_times_.insert(next_time);
  }

  // Add a timer to expire at the next timepoint
  std::chrono::system_clock::time_point next_time = *schedule_times_.begin();
  std::chrono::system_clock::duration time_until_next = next_time - std::chrono::system_clock::now();
  schedule_timer_ = nh_.createTimer(ros::Duration(std::chrono::duration_cast<std::chrono::seconds>(time_until_next).count()), 
    [this](const ros::TimerEvent&)
    { 
      // Call the action
      for (auto & callback : action_callbacks_)
      {
        callback(action_);
      }

      // Add 7 days to the timepoint just called
      std::chrono::system_clock::time_point next_time = *schedule_times_.begin() + std::chrono::hours(24*7);

      // Erase the old timepoint and insert the new one
      schedule_times_.erase(schedule_times_.begin());
      schedule_times_.insert(next_time);

      // Schedule the next timepoin
      std::chrono::system_clock::time_point next_scheduled_time = *schedule_times_.begin();
      std::chrono::system_clock::duration time_until_next = next_scheduled_time - std::chrono::system_clock::now();
      schedule_timer_.setPeriod(ros::Duration(std::chrono::duration_cast<std::chrono::seconds>(time_until_next).count()));
    });
}

std::chrono::system_clock::time_point rios::data_collector::RosDataIngestor::ScheduleEvent::getLastTime() const
{
  return *schedule_times_.rbegin();
}

const std::string& rios::data_collector::RosDataIngestor::ScheduleEvent::getAction() const
{
  return action_;
}
