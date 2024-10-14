#include "outputters/fs_outputter.h"


rios::data_collector::FilesystemOutputter::FilesystemOutputter(const rios::cfg& fs_outputter_config)
: config_(fs_outputter_config)
{

}

bool rios::data_collector::FilesystemOutputter::outputData(std::string episode_name, FormattedData::Ptr data, std::string& message)
{
  // Create the config path if it doesn't exist
  if (!config_["path"])
  {
    message = "No path defined in the filesystem outputter configuration.";
    return false;
  }

  if (!std::filesystem::exists(config_["path"].as<std::string>()))
  {
    std::filesystem::create_directories(config_["path"].as<std::string>());
  }

  return data->asFiles(config_["path"].as<std::string>());
}