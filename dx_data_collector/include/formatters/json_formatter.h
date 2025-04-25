/**
 * @file json_formatter.h
 * @author leo keselman (leo.keselman@rios.ai)
 * @brief JSON formatter implementation
 * @version 0.1
 * @date 2023-10-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __JSON_FORMATTER_H__
#define __JSON_FORMATTER_H__

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <ctime>

#include <dx_rios_yaml/yaml.h>
#include <dx_rios_utils/json/nlohmann/json.hpp>

#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgcodecs.hpp>

#include "interfaces/data_formatter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a formatted data
 * 
 */
class JsonData : public FormattedData
{
public:

  DECLARE_SMART_PTR(JsonData)

public:

  /**
   * @brief Construct a new Json Data object
   * 
   * @param key_json_data The json of keys representing the data
   * @param msg_jsons Map of jsons describing each message
   * @param images A map of images to be saved as png keyed by their names 
   */  
  JsonData(std::shared_ptr<nlohmann::json> key_json_data,
           std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons,  
           std::shared_ptr<std::unordered_map<std::string, cv_bridge::CvImageConstPtr>> images);

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
   * @brief The json data
   * 
   */
  std::shared_ptr<nlohmann::json> key_json_data_;

  /**
   * @brief A map of jsons describing the messages by their uuid
   * 
   */
  std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons_;

  /**
   * @brief A map of images to be saved as png keyed by their names
   * 
   */
  std::shared_ptr<std::unordered_map<std::string, cv_bridge::CvImageConstPtr>> images_;

};

/**
 * @brief Encapsulates a data formatter
 * 
 */
class JsonFormatter : public DataFormatter
{
public:

  DECLARE_SMART_PTR(JsonFormatter)

public:

  /**
   * @brief Construct a new Json Formatter object
   * 
   * @param json_formatter_config The configuration for the formatter
   */ 
  JsonFormatter(const rios::cfg& json_formatter_config);

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
  const rios::cfg& config_;

  /**
   * @brief Store a json value from FieldData
   * 
   * @tparam DataType The type of the data
   * @param json The json to store to
   * @param field_data The data
   * @return true Data was this type and was stored
   * @return false Data was not this type and was not stored
   */
  template <typename DataType>
  bool storeFromVariant(nlohmann::json* json, const FieldData& field_data)
  {
    if (std::holds_alternative<DataType>(field_data))
    {
      *json = std::get<DataType>(field_data);
      return true;
    }

    return false;
  }

};


} /* data_collector */ } /* rios */


#endif /* __JSON_FORMATTER_H__ */
