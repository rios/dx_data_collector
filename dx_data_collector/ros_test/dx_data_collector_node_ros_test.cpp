#include <ros/ros.h>
#include <ros/service_client.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <rosconsole/macros_generated.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <std_msgs/Time.h>

#include <filesystem>
#include <optional>
#include <string>

#include "dx_data_collector_msgs/Snapshot.h"
#include "dx_data_collector_msgs/SnapshotTrigger.h"
#include "gtest/gtest.h"

namespace
{
namespace fs = std::filesystem;

constexpr char kTempTestBagsFolder[] = "/tmp/dx_data_collector_test_tmp_bags";

class DxDataCollectorNodeRosTest : public ::testing::Test
{
protected:
  void SetUp() override { std::filesystem::create_directories(kTempTestBagsFolder); }
  void TearDown() override {}

  std::optional<std::string> GetBagPath()
  {
    int bag_count = 0;
    std::string bag_path;
    for (const auto & entry : fs::directory_iterator(kTempTestBagsFolder)) {
      if (entry.path().extension() == ".bag") {
        bag_path = entry.path();
        bag_count++;
      }
    }
    if (bag_count != 1) {
      ROS_ERROR("Expected exactly one bag file in the folder.");
      return std::nullopt;
    }
    if (bag_path.empty()) {
      ROS_ERROR("Bag path is empty.");
      return std::nullopt;
    }
    return bag_path;
  }

  std::map<std::string, int> GetMessageCounts(const std::string & bag_path)
  {
    rosbag::Bag bag;
    bag.open(bag_path, rosbag::bagmode::Read);

    std::map<std::string, int> message_counts;
    for (const rosbag::MessageInstance m : rosbag::View(bag)) {
      message_counts[m.getTopic()]++;
    }
    bag.close();
    return message_counts;
  }
};

TEST_F(DxDataCollectorNodeRosTest, SnapShotService)
{
  ASSERT_TRUE(fs::is_empty(kTempTestBagsFolder));

  ros::NodeHandle nh("~");

  ros::Publisher pub_cloud = nh.advertise<sensor_msgs::PointCloud2>("/point_cloud_no_filter", 2);
  ros::Publisher pub_cloud_filt = nh.advertise<sensor_msgs::PointCloud2>("/point_cloud_filter", 2);
  ros::Publisher pub_cloud_filt_rel =
    nh.advertise<std_msgs::Time>("/point_cloud_filter_relevant", 2);
  ros::Publisher pub_image_filt = nh.advertise<sensor_msgs::Image>("/image_filter", 2);
  ros::Publisher pub_image_filt_rel = nh.advertise<std_msgs::Time>("/image_filter_relevant", 2);

  ros::ServiceClient snapshot_client =
    nh.serviceClient<dx_data_collector_msgs::Snapshot>("/dx_data_collector/snapshot");
  snapshot_client.waitForExistence();

  ROS_INFO("Publish test messages at 10hz.");
  int count = 0;
  const int MAX_COUNT = 10;
  ros::Rate rate(10);
  ros::Time snapshot_start, relevant_ts_pc, relevant_ts_img;
  while (ros::ok() && count < MAX_COUNT) {
    sensor_msgs::PointCloud2 test_cloud_msg_1, test_cloud_msg_2;
    const ros::Time now = ros::Time::now();
    test_cloud_msg_1.header.stamp = now;
    test_cloud_msg_1.header.seq = count++;
    test_cloud_msg_2.header.stamp = now;
    test_cloud_msg_2.header.seq = count + 1000;
    pub_cloud.publish(test_cloud_msg_1);
    pub_cloud_filt.publish(test_cloud_msg_2);

    sensor_msgs::Image test_image_msg;
    test_image_msg.header.stamp = now + ros::Duration(0.001);
    test_image_msg.header.seq = count + 100;
    pub_image_filt.publish(test_image_msg);

    // Keep track of start time for the snapshot request.
    if (count == 5) {
      snapshot_start = ros::Time::now();
    }
    if (count == 8 || count == 10) {
      std_msgs::Time ts_msg;
      ts_msg.data = now;
      pub_cloud_filt_rel.publish(ts_msg);
    }
    if (count == 9) {
      std_msgs::Time ts_msg;
      ts_msg.data = now + ros::Duration(0.001);
      pub_image_filt_rel.publish(ts_msg);
    }

    ros::spinOnce();
    rate.sleep();
  }
  ROS_INFO("Published all messages.");

  dx_data_collector_msgs::Snapshot snapshot_srv;
  snapshot_srv.request.start_time = snapshot_start;
  snapshot_srv.request.end_time = ros::Time::now();
  snapshot_srv.request.snapshot_name = "collector_filter_test";

  if (snapshot_client.call(snapshot_srv)) {
    ROS_INFO(
      "Response: success:%d, message: %s", snapshot_srv.response.success,
      snapshot_srv.response.message.c_str());
  } else {
    ROS_ERROR("Failed to call service: /dx_data_collector/snapshot");
  }

  // Wait for the bag to be created
  ros::Duration(0.1).sleep();

  const std::optional<std::string> bag_path = GetBagPath();
  ASSERT_TRUE(bag_path.has_value()) << "Bag path is not valid.";
  ROS_INFO("Confirmed single rosbag file: %s", bag_path->c_str());

  const std::map<std::string, int> message_counts = GetMessageCounts(*bag_path);

  EXPECT_EQ(message_counts.size(), 3);
  EXPECT_TRUE(message_counts.find("/point_cloud_no_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/point_cloud_no_filter"), 6);

  EXPECT_TRUE(message_counts.find("/point_cloud_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/point_cloud_filter"), 2);

  EXPECT_TRUE(message_counts.find("/image_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/image_filter"), 1);

  fs::remove(*bag_path);
}

// Test using the fire-and-forget snapshot trigger topic
TEST_F(DxDataCollectorNodeRosTest, SnapShotTrigger)
{
  ASSERT_TRUE(fs::is_empty(kTempTestBagsFolder));

  ros::NodeHandle nh("~");

  ros::Publisher pub_cloud = nh.advertise<sensor_msgs::PointCloud2>("/point_cloud_no_filter", 2);
  ros::Publisher pub_cloud_filt = nh.advertise<sensor_msgs::PointCloud2>("/point_cloud_filter", 2);
  ros::Publisher pub_cloud_filt_rel =
    nh.advertise<std_msgs::Time>("/point_cloud_filter_relevant", 2);
  ros::Publisher pub_image_filt = nh.advertise<sensor_msgs::Image>("/image_filter", 2);
  ros::Publisher pub_image_filt_rel = nh.advertise<std_msgs::Time>("/image_filter_relevant", 2);

  ros::Publisher snapshot_trigger_pub =
    nh.advertise<dx_data_collector_msgs::SnapshotTrigger>("/dx_data_collector/snapshot_trigger", 2);

  ROS_INFO("Publish test messages at 10hz.");
  int count = 0;
  const int MAX_COUNT = 10;
  ros::Rate rate(10);
  ros::Time snapshot_start, relevant_ts_pc, relevant_ts_img;
  while (ros::ok() && count < MAX_COUNT) {
    sensor_msgs::PointCloud2 test_cloud_msg_1, test_cloud_msg_2;
    const ros::Time now = ros::Time::now();
    test_cloud_msg_1.header.stamp = now;
    test_cloud_msg_1.header.seq = count++;
    test_cloud_msg_2.header.stamp = now;
    test_cloud_msg_2.header.seq = count + 1000;
    pub_cloud.publish(test_cloud_msg_1);
    pub_cloud_filt.publish(test_cloud_msg_2);

    sensor_msgs::Image test_image_msg;
    test_image_msg.header.stamp = now + ros::Duration(0.001);
    test_image_msg.header.seq = count + 100;
    pub_image_filt.publish(test_image_msg);

    // Keep track of start time for the snapshot request.
    if (count == 5) {
      snapshot_start = ros::Time::now();
    }
    if (count == 8 || count == 10) {
      std_msgs::Time ts_msg;
      ts_msg.data = now;
      pub_cloud_filt_rel.publish(ts_msg);
    }
    if (count == 9) {
      std_msgs::Time ts_msg;
      ts_msg.data = now + ros::Duration(0.001);
      pub_image_filt_rel.publish(ts_msg);
    }

    ros::spinOnce();
    rate.sleep();
  }
  ROS_INFO("Published all messages.");

  dx_data_collector_msgs::SnapshotTrigger trigger;
  trigger.start_time = snapshot_start;
  trigger.end_time = ros::Time::now();
  trigger.snapshot_name = "collector_filter_test";

  snapshot_trigger_pub.publish(trigger);
  ROS_INFO("Published snapshot trigger.");

  // Wait for the bag to be created
  ros::Duration(0.1).sleep();

  const std::optional<std::string> bag_path = GetBagPath();
  ASSERT_TRUE(bag_path.has_value()) << "Bag path is not valid.";
  ROS_INFO("Confirmed single rosbag file: %s", bag_path->c_str());

  const std::map<std::string, int> message_counts = GetMessageCounts(*bag_path);

  EXPECT_EQ(message_counts.size(), 3);
  EXPECT_TRUE(message_counts.find("/point_cloud_no_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/point_cloud_no_filter"), 6);

  EXPECT_TRUE(message_counts.find("/point_cloud_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/point_cloud_filter"), 2);

  EXPECT_TRUE(message_counts.find("/image_filter") != message_counts.end());
  EXPECT_EQ(message_counts.at("/image_filter"), 1);

  fs::remove(*bag_path);
}
}  // namespace

int main(int argc, char * argv[])
{
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "rostest_node");
  return RUN_ALL_TESTS();
}
