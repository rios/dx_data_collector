#include "formatters/rosbag_formatter.h"

#include <rosbag/bag.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <topic_tools/shape_shifter.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <optional>

#include "ros_msg_parser/utils/shape_shifter.hpp"

namespace
{

// This is a template function to extract timestamp from any message type with a header.
template <typename MsgType>
std::optional<ros::Time> ExtractTimeStamp(const topic_tools::ShapeShifter & ss_msg)
{
  typename MsgType::ConstPtr msg_typed = ss_msg.instantiate<MsgType>();
  if (msg_typed) {
    return msg_typed->header.stamp;
  } else {
    return std::nullopt;
  }
}

std::string getCurrentDateTime()
{
  std::time_t now = std::time(nullptr);
  std::tm * tm_now = std::localtime(&now);

  std::ostringstream oss;
  oss << std::put_time(tm_now, "%Y-%m-%d-%H-%M-%S");
  return oss.str();
}
}  // namespace

rios::data_collector::RosbagFormatter::RosbagFormatter(const rios::cfg & rosbag_formatter_config)
: config_(rosbag_formatter_config)
{
}

rios::data_collector::FormattedData::Ptr rios::data_collector::RosbagFormatter::formatData(
  std::deque<DxRosMsg::Ptr> & data_queue,
  std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name,
  const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors)
{
  // Create a temporary directory to store the bag file - use UUID as this formatter could run
  // simultaneously from different threads
  std::filesystem::path temp_storage =
    "/tmp/" + boost::uuids::to_string(boost::uuids::random_generator()()) + "/";
  std::filesystem::create_directory(temp_storage);

  std::string bag_name = getCurrentDateTime() + "_" + snapshot_name + ".bag";
  rosbag::Bag out_bag(temp_storage / bag_name, rosbag::BagMode::Write);

  std::unordered_map<std::string, size_t> topic_reject_count;

  for (auto & msg : data_queue) {
    topic_tools::ShapeShifter shape_shifter;
    msg->toShapeShifter(shape_shifter);
    const std::string data_type = shape_shifter.getDataType();
    if (data_type == "sensor_msgs/PointCloud2" || data_type == "sensor_msgs/Image") {
      const TopicIngestor::Ptr ingestor = topic_ingestors->at(msg->topicName());

      if (ingestor->timestampFilterEnabled()) {
        std::optional<ros::Time> time_stamp;
        if (data_type == "sensor_msgs/PointCloud2") {
          time_stamp = ExtractTimeStamp<sensor_msgs::PointCloud2>(shape_shifter);
        } else if (data_type == "sensor_msgs/Image") {
          time_stamp = ExtractTimeStamp<sensor_msgs::Image>(shape_shifter);
        } else {
          ROS_ERROR_STREAM("Unsupported message type for timestamp extraction: " << data_type);
        }

        if (time_stamp) {
          if (!ingestor->isTimeStampRelevant(*time_stamp)) {
            topic_reject_count[msg->topicName()]++;
            continue;
          }
        } else {
          ROS_WARN("Failed to extract timestamp");
        }
      }
    }

    out_bag.write(msg->topicName(), msg->timeRecvd(), shape_shifter);
  }

  ROS_INFO("Timestamp filter discarded messages counts:");
  for (const auto & [topic_name, reject_count] : topic_reject_count) {
    ROS_INFO_STREAM("  - " << topic_name << ": " << reject_count);
  }

  // Clear the relevant timestamps for all topic ingestors
  for (const auto & [topic_name, ingestor] : *topic_ingestors) {
    if (ingestor->timestampFilterEnabled()) {
      ingestor->clearRelevantTimestamps();
    }
  }

  return std::make_shared<rios::data_collector::RosbagData>(temp_storage / bag_name);
}

rios::data_collector::RosbagData::RosbagData(std::filesystem::path bag_location)
: bag_location_(bag_location)
{
}

rios::data_collector::RosbagData::~RosbagData()
{
  // Since the data is no longer needed, we can delete it from the fs
  std::filesystem::remove(bag_location_);
}

bool rios::data_collector::RosbagData::asFiles(std::filesystem::path path)
{
  std::error_code error_code;
  std::filesystem::copy(bag_location_, path, error_code);

  return !error_code;
}
