/**
 * @file dummy_formatter.h
 * @author leo keselman (leo.keselman@rios.ai)
 * @brief dummy formatter implementation
 * @version 0.1
 * @date 2023-10-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __DUMMY_FORMATTER_H__
#define __DUMMY_FORMATTER_H__


#include <dx_rios_yaml/yaml.h>

#include "interfaces/data_formatter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a formatted data
 * 
 */
class DummyData : public FormattedData
{
public:

  DECLARE_SMART_PTR(DummyData)

public:

  /**
   * @brief Construct a new Json Data object
   * 
   */  
  DummyData() {}

  /**
   * @brief Store formatted data as files at a path
   * 
   * @param path The path to store the files 
   * @return true Successfully stored
   * @return false Failed to store 
   */
  virtual bool asFiles(std::filesystem::path path) override {return true;}

};

/**
 * @brief Encapsulates a data formatter
 * 
 */
class DummyFormatter : public DataFormatter
{
public:

  DECLARE_SMART_PTR(DummyFormatter)

public:

  /**
   * @brief Construct a new Dummy Formatter object
   * 
   * @param dummy_formatter_config The configuration for the formatter
   */ 
  DummyFormatter(const rios::cfg& dummy_formatter_config) {}

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
    const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors) override
  {
    return std::make_shared<DummyData>();
  };
};

} /* data_collector */ } /* rios */


#endif /* __DUMMY_FORMATTER_H__ */
