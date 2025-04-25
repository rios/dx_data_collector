/**
 * @file data_formatter.h
 * @author leo keselman (leo.keselman@rios.ai)
 * @brief Data formatter interface definition
 * @version 0.1
 * @date 2023-10-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __DATA_FORMATTER_H__
#define __DATA_FORMATTER_H__

#include <filesystem>

#include "data_types/dx_ros_msg.h"
#include "ros_ingestion.h"

namespace rios { namespace data_collector {

/**
 * @brief Encapsulates formatted data
 * 
 */
class FormattedData
{
public:

  DECLARE_SMART_PTR(FormattedData)

public:

  /**
   * @brief Store formatted data as files at a path
   * 
   * @param path The path to store the files 
   * @return true Successfully stored
   * @return false Failed to store 
   */
  virtual bool asFiles(std::filesystem::path path) = 0;

};

/**
 * @brief Encapsulates a data formatter
 * 
 */
class DataFormatter
{
public:

  DECLARE_SMART_PTR(DataFormatter)

public:
  /**
   * @brief Format the data as desired
   *
   * @param data_queue The queue of buffered messages to format for output.
   * @param params The parameters coming from ROS
   * @param snapshot_name The name of the snapshot to format
   * @param topic_ingestors Map of topic ingestors used for timestamp filtering.
   * @return the formatted data
   */
  virtual FormattedData::Ptr formatData(
    std::deque<DxRosMsg::Ptr> & data_queue,
    std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name,
    const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors) = 0;
};


} /* data_collector */ } /* rios */


#endif /* __DATA_FORMATTER_H__ */
