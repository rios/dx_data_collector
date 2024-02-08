#include "outputters/fs_outputter.h"


rios::data_collector::FilesystemOutputter::FilesystemOutputter(const rios::cfg& fs_outputter_config)
: config_(fs_outputter_config)
{

}

bool rios::data_collector::FilesystemOutputter::outputData(std::string episode_name, FormattedData::Ptr data, std::string& message)
{
  return data->asFiles(config_["path"].as<std::string>());
}