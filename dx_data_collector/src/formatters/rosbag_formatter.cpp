#include "formatters/rosbag_formatter.h"

namespace
{
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
  std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name)
{
  // Create a temporary directory to store the bag file - use UUID as this formatter could run
  // simultaneously from different threads
  std::filesystem::path temp_storage =
    "/tmp/" + boost::uuids::to_string(boost::uuids::random_generator()()) + "/";
  std::filesystem::create_directory(temp_storage);

  std::string bag_name = getCurrentDateTime() + "_" + snapshot_name + ".bag";
  rosbag::Bag out_bag(temp_storage / bag_name, rosbag::BagMode::Write);

  for (auto & msg : data_queue) {
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
