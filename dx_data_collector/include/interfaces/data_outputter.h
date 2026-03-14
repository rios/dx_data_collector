/**
 * @file data_outputter.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief Data outputter interface definition
 * @version 0.1
 * @date 2023-10-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __DATA_OUTPUTTER_H__
#define __DATA_OUTPUTTER_H__

#include <filesystem>

#include <ros/ros.h>

#include "data_types/dx_ros_msg.h"
#include "interfaces/data_formatter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a data formatter
 * 
 */
class DataOutputter
{
public:

  DECLARE_SMART_PTR(DataOutputter)

public:

  /**
   * @brief Output the data
   * 
   * @param episode_name The name of the episode
   * @param data The data to output
   * @param message Any message regarding the output
   * @return true Output succeeded
   * @return false Output failed
   */
  virtual bool outputData(std::string episode_name, FormattedData::Ptr data, std::string& message) = 0;

};


} /* data_collector */ } /* rios */


#endif /* __DATA_OUTPUTTER_H__ */
