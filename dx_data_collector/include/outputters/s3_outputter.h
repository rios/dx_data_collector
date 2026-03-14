/**
 * @file s3_outputter.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief S3 outputter implementation
 * @version 0.1
 * @date 2023-10-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __S3_OUTPUTTER_H__
#define __S3_OUTPUTTER_H__

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/transfer/TransferManager.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <dx_rios_yaml/yaml.h>

#include "interfaces/data_outputter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a data formatter
 * 
 */
class S3Outputter : public DataOutputter
{
public:

  DECLARE_SMART_PTR(S3Outputter)

public:

  /**
   * @brief Construct a new S3 Outputter object
   * 
   * @param s3_outputter_config The configuration for the outputter
   */ 
  S3Outputter(const rios::cfg& s3_outputter_config);

  /**
   * @brief Destroy the S3Outputter object
   * 
   */
  ~S3Outputter();

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
   * @brief The configuration for the formatter
   */
  const rios::cfg& config_;

  /**
   * @brief Options for AWS SDK
   * 
   */
  Aws::SDKOptions aws_options_;

  /**
   * @brief S3 client
   * 
   */
  std::shared_ptr<Aws::S3::S3Client> s3_client_;

  /**
   * @brief Transfer execution
   * 
   */
  std::shared_ptr<Aws::Utils::Threading::PooledThreadExecutor> transfer_executor_;

  /**
   * @brief Transfer configuration
   * 
   */
  Aws::Transfer::TransferManagerConfiguration transfer_config_;

  /**
   * @brief Transfer manager
   * 
   */
  static std::shared_ptr<Aws::Transfer::TransferManager> transfer_manager_;

  /**
   * @brief Base path where data is temporarily output before being uploaded
   * 
   */
  std::filesystem::path data_base_path_;

  /**
   * @brief Whether everything was set up properly
   * 
   */
  bool s3_connected_;

};


} /* data_collector */ } /* rios */


#endif /* __S3_OUTPUTTER_H__ */
