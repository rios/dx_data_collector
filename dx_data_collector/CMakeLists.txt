cmake_minimum_required(VERSION 3.0.2)
project(dx_data_collector)

add_compile_options(-std=c++17)

find_package(
  catkin REQUIRED
  COMPONENTS 
    dx_data_collector_msgs
    roscpp
    roscpp_serialization
    rosbag_storage
    std_msgs
    dx_sysmon_sdk
    cv_bridge
    rosbag
    tf2_ros
)

find_package(aws-cpp-sdk-core REQUIRED aws-cpp-sdk-s3 aws-cpp-sdk-transfer)

add_executable(dx_data_collector_node src/dx_data_collector_node.cpp
                                      src/ros_ingestion.cpp
                                      src/data_types/dx_ros_msg.cpp
                                      src/data_types/field_data.cpp
                                      src/formatters/json_formatter.cpp
                                      src/formatters/rosbag_formatter.cpp
                                      src/outputters/s3_outputter.cpp
                                      src/outputters/fs_outputter.cpp
                                      src/ros_msg_parser/ros_type.cpp
                                      src/ros_msg_parser/ros_message.cpp
                                      src/ros_msg_parser/ros_parser.cpp
                                      src/ros_msg_parser/ros_field.cpp
                                      src/ros_msg_parser/stringtree_leaf.cpp)

target_link_libraries(
  dx_data_collector_node ${catkin_LIBRARIES} aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-transfer
)

catkin_package()

include_directories(
  include 
  include/dx_data_collector
  ${catkin_INCLUDE_DIRS}
  ${AWSSDK_INCLUDE_DIRS}
)

install(DIRECTORY launch config DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS dx_data_collector_node RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})