#include "outputters/s3_outputter.h"

std::shared_ptr<Aws::Transfer::TransferManager> rios::data_collector::S3Outputter::transfer_manager_;

rios::data_collector::S3Outputter::S3Outputter(const rios::cfg& s3_outputter_config)
: config_(s3_outputter_config)
, transfer_executor_(Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("executor", 25))
, transfer_config_(transfer_executor_.get())
, s3_connected_(false)
{
  // Start the AWS API
  Aws::InitAPI(aws_options_);

  // Set up the S3 client

  Aws::Auth::DefaultAWSCredentialsProviderChain provider;
  Aws::Auth::AWSCredentials creds = provider.GetAWSCredentials();
  ROS_INFO_STREAM("AWS SDK Looking for credentials in chain. Looking for profile [default]");
  if (creds.IsEmpty()) 
  {
      ROS_ERROR_STREAM("S3 Client could not find authentication!");
      return;
  }
  ROS_INFO_STREAM("Found S3 authentication");

  Aws::S3::S3ClientConfiguration s3_client_config;
  s3_client_config.endpointOverride = config_["endpoint_url"].as<std::string>();
  s3_client_ = Aws::MakeShared<Aws::S3::S3Client>("S3Client", creds, Aws::MakeShared<Aws::S3::S3EndpointProvider>(Aws::S3::S3Client::ALLOCATION_TAG), s3_client_config);

  Aws::S3::Model::ListBucketsOutcome bucket_list_outcome = s3_client_->ListBuckets();

  if (!bucket_list_outcome.IsSuccess()) 
  {
      ROS_ERROR_STREAM("S3 Client authenticated but could not list buckets: " << bucket_list_outcome.GetError());
      return;
  }

  Aws::S3::Model::ListObjectsRequest objects_request;
  objects_request.WithBucket(Aws::String(config_["bucket"].as<std::string>()));

  Aws::S3::Model::ListObjectsOutcome obj_list_outcome = s3_client_->ListObjects(objects_request);

  if (!obj_list_outcome.IsSuccess()) 
  {
      ROS_ERROR_STREAM("Connection to S3 established but could not find contents of requested bucket (" << config_["bucket"].as<std::string>() << "): " << obj_list_outcome.GetError());
      return;
  }

  ROS_INFO_STREAM("S3 connection succeeded");

  // Set up the transfer client
  transfer_config_.s3Client = s3_client_;
  transfer_config_.transferInitiatedCallback = 
    [](const Aws::Transfer::TransferManager * transfer_manager, const std::shared_ptr<const Aws::Transfer::TransferHandle> transfer_handle)
    {
      ROS_DEBUG_STREAM("Starting transfer of " << transfer_handle->GetTargetFilePath());
    }
  ;

  transfer_manager_= Aws::Transfer::TransferManager::Create(transfer_config_);

  // Set up base paths for local data temporary storage
  if (config_["local_base_path"])
  {
    data_base_path_ = config_["local_base_path"].as<std::string>();
  }
  else
  {
    data_base_path_ = "/tmp";
  }

  s3_connected_ = true;
}

rios::data_collector::S3Outputter::~S3Outputter()
{
  Aws::ShutdownAPI(aws_options_);
}

bool rios::data_collector::S3Outputter::outputData(std::string episode_name, FormattedData::Ptr data, std::string& message)
{
  if (!s3_connected_)
  {
    message = "Cannot output data to S3 as we could not connect";
    return false;
  }

  // Output the data to a folder named by a transfer uuid
  std::string transfer_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
  std::filesystem::path transfer_data_folder = data_base_path_ / transfer_uuid;
  if (!std::filesystem::create_directory(transfer_data_folder ))
  {
    message = "Could not create local data directory " + transfer_data_folder.string() + ". Cannot upload requested data";
    return false;
  }

  data->asFiles(transfer_data_folder);
  
  Aws::String aws_folder_name(transfer_data_folder.string());
  std::string bucket_path = config_["env"].as<std::string>() + "/" + config_["area"].as<std::string>() + "/episode/" + episode_name;
  transfer_manager_->UploadDirectory(aws_folder_name, Aws::String(config_["bucket"].as<std::string>()), Aws::String(bucket_path), Aws::Http::HeaderValueCollection());

  return true;
}