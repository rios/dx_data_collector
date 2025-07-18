/**
 * @file data_collector_commander.h
 * @author Leo Keselman (leo.keselman@rios.ai)
 * @brief Commander for triggering data collection
 * @version 0.1
 * @date 2023-10-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __RIOS_DATA_COLLECTOR_COMMANDER_H__
#define __RIOS_DATA_COLLECTOR_COMMANDER_H__

#include <dx_data_collector_msgs/Snapshot.h>
#include <dx_data_collector_msgs/SnapshotTrigger.h>
#include <dx_rios_utils/base.h>
#include <ros/ros.h>

#include <optional>

namespace rios { namespace data_collection {


class DataCollectorCommander
{

public:

    DECLARE_SMART_PTR(DataCollectorCommander)
    
public:

    /**
     * @brief Construct a new Data Collector Commander object
     * 
     * @param collector_id The ID of the collector. If not specified, assumed to be the default
     */
    DataCollectorCommander(const std::string& collector_id = "dx_data_collector");

    /**
     * @brief Record a data snapshot
     * 
     * @param time_s The time back to record. Optional - defaults to saving the entire buffer
     * @param snapshot_name A name for the snapshot. Optional - defaults to episode name in collector config
     * @return true Snapshot recorded
     * @return false Snapshot could not be recorded
     */
    bool takeSnapshot(std::optional<double> time_s = std::nullopt, std::optional<const std::string> snapshot_name = std::nullopt);

    /**
     * @brief Record a data snapshot with fire and forget semantics using the topic interface
     *
     * @param time_s The time back to record. Optional - defaults to saving the entire buffer
     * @param snapshot_name A name for the snapshot. Optional - defaults to episode name in collector config
     */
    void takeSnapshotFF(
      std::optional<double> time_s = std::nullopt,
      std::optional<const std::string> snapshot_name = std::nullopt);

    /**
     * @brief Start a data capture
     * 
     * @return true Data capture started
     * @return false Data capture could not be started
     */
    bool startRecording();

    /**
     * @brief Stop recording and save data snapshot
     * 
     * @param snapshot_name The name of the snapshot. Optional - defaults to episode name in config
     * @return true Recording stopped and saved
     * @return false Recording could not be saved
     */
    bool stopRecording(std::optional<const std::string> snapshot_name = std::nullopt);

    /**
     * @brief Stop recording and save data snapshot with fire and forget semantics using the topic interface
     *
     * @param snapshot_name The name of the snapshot. Optional - defaults to episode name in config
     */
    bool stopRecordingFF(std::optional<const std::string> snapshot_name = std::nullopt);

  private:
    /**
     * @brief Node handle
     * 
     */
    ros::NodeHandle nh_;

    /**
     * @brief Service to perform data snapshot
     * 
     */
    ros::ServiceClient snapshot_service_;

    /**
     * @brief Publisher for fire and forget snapshot trigger
     *
     */
    ros::Publisher snapshot_trigger_pub_;

    /**
     * @brief The Id of the collector
     * 
     */
    std::string collector_id_;  

    /**
     * @brief Recording start time, if it was started
     * 
     */
    std::optional<ros::Time> record_start_time_;

};


} /* data_collection */ } /* rios */


#endif /* __RIOS_DATA_COLLECTOR_COMMANDER_H__ */
