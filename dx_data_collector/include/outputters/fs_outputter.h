/**
 * @file fs_outputter.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief filesystem outputter implementation
 * @version 0.1
 * @date 2023-10-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __FS_OUTPUTTER_H__
#define __FS_OUTPUTTER_H__

#include <filesystem>

#include <dx_rios_yaml/yaml.h>

#include "interfaces/data_outputter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a data formatter
 * 
 */
class FilesystemOutputter : public DataOutputter
{
public:

  DECLARE_SMART_PTR(FilesystemOutputter)

public:

  /**
   * @brief Construct a new S3 Outputter object
   * 
   * @param fs_outputter_config The configuration for the outputter
   */ 
  FilesystemOutputter(const rios::cfg& fs_outputter_config);

  /**
   * @brief Output the data
   *
   * @param episode_name Name of the episode 
   * @param data The data to output
   * @param message Any message regarding the output
   * @return true Output succeeded
   * @return false Output failed
   */
  virtual bool outputData(std::string episode_name, FormattedData::Ptr data, std::string& message) override;

private:
  /**
   * @brief The configuration for the outputter
   */
  const rios::cfg& config_;

};


} /* data_collector */ } /* rios */


#endif /* __FS_OUTPUTTER_H__ */
