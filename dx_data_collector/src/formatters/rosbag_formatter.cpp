#include "formatters/rosbag_formatter.h"

rios::data_collector::RosbagFormatter::RosbagFormatter(const rios::cfg& rosbag_formatter_config)
: config_(rosbag_formatter_config)
{

}

rios::data_collector::FormattedData::Ptr rios::data_collector::RosbagFormatter::formatData(std::deque<DxRosMsg::Ptr>& data_queue, std::string snapshot_name)
{
  // Create a temporary directory to store the bag file - use UUID as this formatter could run simultaneously from different threads
  std::filesystem::path temp_storage = "/tmp/" + boost::uuids::to_string(boost::uuids::random_generator()()) + "/";
  std::filesystem::create_directory(temp_storage);

  std::time_t t = std::time(0); 
  std::tm* now = std::localtime(&t);
  std::string year = std::to_string(now->tm_year + 1900);
  std::string month = std::to_string(now->tm_mon + 1);
  if (now->tm_mon < 9) month = "0" + month;
  std::string day = std::to_string(now->tm_mday);
  if (now->tm_mday < 10) day = "0" + day;
  std::string hour = std::to_string(now->tm_hour);
  if (now->tm_hour < 10) hour = "0" + hour;
  std::string min = std::to_string(now->tm_min);
  if (now->tm_min < 10) min = "0" + min;
  std::string sec = std::to_string(now->tm_sec);
  if (now->tm_sec < 10) sec = "0" + sec;

  std::string bag_name = snapshot_name + "-" + month + "-" + day + "-" + year + "-" + hour + "-" + min + "-" + sec + ".bag";
  rosbag::Bag out_bag(temp_storage / bag_name, rosbag::BagMode::Write);

  for (auto & msg : data_queue)
  {
    topic_tools::ShapeShifter shape_shifter;
    msg->toShapeShifter(shape_shifter);

    out_bag.write(msg->topicName(), msg->timeRecvd(), shape_shifter);
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
