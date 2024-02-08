#ifndef _DX_ROS_MSG_FIELD_DATA_H__
#define _DX_ROS_MSG_FIELD_DATA_H__

#include <variant>
#include <ros_msg_parser/utils/span.hpp>
#include <ros/ros.h>

#include <dx_rios_utils/string_utils.h>

namespace rios { namespace data_collector {

  std::vector<std::string> fieldNameAsTokens(std::string field_name);

  /**
   * @brief Type of data held in a field
   * 
   */
  typedef std::variant<std::string,
                       nonstd::span<const uint8_t>, 
                       double,
                       bool,
                       char,
                       uint8_t,
                       uint16_t,
                       uint32_t,
                       uint64_t,
                       int8_t,
                       int16_t,
                       int32_t,
                       int64_t,
                       float,
                       ros::Time,
                       ros::Duration> FieldData;

} /* data_collector */ } /* rios */

#endif /* _DX_ROS_MSG_FIELD_DATA_H__ */