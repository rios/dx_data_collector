/**
 * @file rosbag_formatter.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief Rosbag formatter implementation
 * @version 0.1
 * @date 2023-10-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef __ROSBAG_FORMATTER_H__
#define __ROSBAG_FORMATTER_H__

#include <dx_rios_yaml/yaml.h>
#include <ros/ros.h>

#include <ctime>
#include <filesystem>

#include "interfaces/data_formatter.h"

namespace rios
{
namespace data_collector
{

/**
 * @brief Encapsulates a formatted data
 *
 */
class RosbagData : public FormattedData
{
public:
  DECLARE_SMART_PTR(RosbagData)

public:
  /**
   * @brief Construct a new Rosbag Data object
   *
   * @param bag_location The location of the bag on the local fs
   */
  RosbagData(std::filesystem::path bag_location);

  /**
   * @brief Destroy the Rosbag Data object
   *
   */
  ~RosbagData();

  /**
   * @brief Store formatted data as files at a path
   *
   * @param path The path to store the files
   * @return true Successfully stored
   * @return false Failed to store
   */
  virtual bool asFiles(std::filesystem::path path) override;

private:
  /**
   * @brief The location of the bag on the local fs
   *
   */
  std::filesystem::path bag_location_;
};

/**
 * @brief Encapsulates a data formatter
 *
 */
class RosbagFormatter : public DataFormatter
{
public:
  DECLARE_SMART_PTR(RosbagFormatter)

public:
  /**
   * @brief Construct a new Json Formatter object
   *
   * @param rosbag_formatter_config The configuration for the formatter
   */
  RosbagFormatter(const rios::cfg & rosbag_formatter_config);

  /**
   * @brief Format the data as desired
   *
   * @param data_queue The queue of messages
   * @param params The parameters coming from ROS
   * @param snapshot_name The name of the snapshot to format
   * @return true Formatting successful
   * @return false Formatting failed
   */
  virtual FormattedData::Ptr formatData(
    std::deque<DxRosMsg::Ptr> & data_queue,
    std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name,
    const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors) override;

private:
  /**
   * @brief The configuration for the formatter
   */
  const rios::cfg & config_;
};

}  // namespace data_collector
}  // namespace rios

#endif /* __ROSBAG_FORMATTER_H__ */
