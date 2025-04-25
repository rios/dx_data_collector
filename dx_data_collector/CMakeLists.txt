cmake_minimum_required(VERSION 3.16)
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
    sensor_msgs
    dx_sysmon_sdk
    cv_bridge
    rosbag
    tf2_ros
)

find_package(aws-cpp-sdk-core REQUIRED aws-cpp-sdk-s3 aws-cpp-sdk-transfer)
find_package(VPL REQUIRED)

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)
find_library(BZ2_LIBRARY bz2)
find_library(ZLIB_LIBRARY z)
find_library(LZMA_LIBRARY lzma)
find_library(VA_LIBRARY va)
find_library(SWRESAMPLE_LIBRARY swresample)
find_library(VPL_LIBRARY vpl)
find_library(DRM_LIBRARY drm)
find_library(X11_LIBRARY X11)
find_library(VA_DRM_LIBRARY va-drm)
find_library(VA_X11_LIBRARY va-x11)
find_library(LIBX264 x264)
find_library(MFX_LIBRARY mfx HINTS /opt/intel/mediasdk/lib)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS roscpp std_msgs sensor_msgs rosbag rosbag_storage
)

include_directories(
  include
  include/dx_data_collector
  ${catkin_INCLUDE_DIRS}
  ${AWSSDK_INCLUDE_DIRS}
  /usr/include/vpl
)

add_executable(dx_data_collector_node src/dx_data_collector_node.cpp
                                      src/ros_ingestion.cpp
                                      src/data_types/dx_ros_msg.cpp
                                      src/data_types/field_data.cpp
                                      src/formatters/json_formatter.cpp
                                      src/formatters/rosbag_formatter.cpp
                                      src/formatters/ia_formatter.cpp
                                      src/outputters/s3_outputter.cpp
                                      src/outputters/fs_outputter.cpp
                                      src/ros_msg_parser/ros_type.cpp
                                      src/ros_msg_parser/ros_message.cpp
                                      src/ros_msg_parser/ros_parser.cpp
                                      src/ros_msg_parser/ros_field.cpp
                                      src/ros_msg_parser/stringtree_leaf.cpp)

target_link_libraries(
  dx_data_collector_node ${catkin_LIBRARIES}
                         aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-transfer
                         ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY}
                         ${BZ2_LIBRARY} ${ZLIB_LIBRARY} ${LZMA_LIBRARY} ${VA_LIBRARY} ${SWRESAMPLE_LIBRARY}
                         ${VPL_LIBRARY} ${DRM_LIBRARY} ${X11_LIBRARY} ${VA_DRM_LIBRARY} ${VA_X11_LIBRARY}
                         ${MFX_LIBRARY} ${LIBX264}
)

install(DIRECTORY launch config DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS dx_data_collector_node RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})


if(TESTING)
  find_package(rostest REQUIRED)
  find_package(GTest REQUIRED)

  # Required by gtest
  find_package(Threads REQUIRED)

  include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
    mocks
  )

  if(UNIT_TESTS)
  endif()

  if(INTEGRATION_TESTS)
    add_executable(${PROJECT_NAME}_node_ros_test ros_test/dx_data_collector_node_ros_test.cpp)


    target_link_libraries(
      ${PROJECT_NAME}_node_ros_test
      ${catkin_LIBRARIES}
      ${GTEST_LIBRARIES}
      ${CMAKE_THREAD_LIBS_INIT}
    )

    install(TARGETS
      ${PROJECT_NAME}_node_ros_test
      RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
    )

    # Add ROS tests using rostest here
    add_rostest(ros_test/dx_data_collector_node.test)

    # Copy over test launch files to the install space
    install(DIRECTORY ros_test/
      DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
      FILES_MATCHING PATTERN "*.test"
    )
  endif()
endif()
