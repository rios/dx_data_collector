/**
 * @file hit1_sysmon_checker.cpp
 * @author Leo Keselman (leo.keselman@rios.ai)
 * @brief Hit1 Sysmon Checker Node
 * @version 0.1
 * @date 2022-08-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <ros/ros.h>

/**
 * @brief Main loop of the checker
 *
 * @param argc
 */
int main(int argc, char** argv)
{
    // Initializes ROS, and sets up a node
    ros::init(argc, argv, "hit1_sysmon_checker");
    ros::NodeHandle nh("~");
    
    ros::spin();
}
