#include "data_types/field_data.h"

std::vector<std::string> rios::data_collector::fieldNameAsTokens(std::string field_name)
{
  return rios::utils::getTokens(field_name, "/");
}
