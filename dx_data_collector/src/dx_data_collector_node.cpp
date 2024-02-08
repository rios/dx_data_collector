/**
 * @file dx_data_collector_node.cpp
 * @author Leo Keselman (leo.keselman@rios.ai)
 * @brief Main Node of the data collector
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <ros/ros.h>

#include <dx_rios_yaml/yaml.h>
#include <dx_sysmon_sdk/client.h>

#include "ros_ingestion.h"

#include "formatters/json_formatter.h"
#include "formatters/rosbag_formatter.h"

#include "outputters/s3_outputter.h"
#include "outputters/fs_outputter.h"

/**
 * Define implemented data formatters here
 */
template<typename FormatterType>
FormatterType* createFormatter(const rios::cfg& formatter_config) {return new FormatterType(formatter_config);}

const std::map<std::string, std::function<rios::data_collector::DataFormatter* (const rios::cfg&)>> cpp_formatters = 
{
    {"json", &createFormatter<rios::data_collector::JsonFormatter>}, // The key matches the 'type' in the config
    {"rosbag", &createFormatter<rios::data_collector::RosbagFormatter>}
};

/**
 * Define implemented data outputters here
 */
template<typename OutputterType>
OutputterType* createOutputter(const rios::cfg& output_config) {return new OutputterType(output_config);}

const std::map<std::string, std::function<rios::data_collector::DataOutputter* (const rios::cfg&)>> cpp_outputters = 
{
    {"s3", &createOutputter<rios::data_collector::S3Outputter>}, // The key matches the 'type' in the config
    {"filesystem", &createOutputter<rios::data_collector::FilesystemOutputter>}
};

int setupPipelines(rios::cfg& collection_config,
                   rios::data_collector::RosDataIngestor& ingestor,
                   std::unordered_map<std::string, rios::cfg>& formatters_configured,
                   std::unordered_map<std::string, rios::cfg>& outputters_configured)
{
    // Set up the data collection pipelines
    int pipeline_num = 0;
    int pipelines_setup = 0;
    for (const auto & pipeline : collection_config["pipelines"])
    {
        pipeline_num++;
        if (!pipeline["formatter"])
        {
            ROS_ERROR_STREAM("No 'formatter' defined for pipeline " << pipeline_num << ". It will not be setup.");
            continue;
        }
        if (!pipeline["outputter"])
        {
            ROS_ERROR_STREAM("No 'output' defined for pipeline " << pipeline_num << ". It will not be setup.");
            continue;
        }

        std::string formatter_name = pipeline["formatter"].as<std::string>();
        std::string outputter_name = pipeline["outputter"].as<std::string>();

        if (!formatters_configured.count(formatter_name))
        {   
            ROS_ERROR_STREAM("No formatter named " << formatter_name << " has been defined in config file 'formatters' section. Typo?");
            continue;
        }

        if (!outputters_configured.count(outputter_name))
        {   
            ROS_ERROR_STREAM("No outputter named " << outputter_name << " has been defined in config file 'output' section. Typo?");
            continue;
        }

        std::string formatter_type = formatters_configured[formatter_name]["type"].as<std::string>();
        std::string outputter_type = outputters_configured[outputter_name]["type"].as<std::string>();


        if (!cpp_formatters.count(formatter_type))
        {
            ROS_ERROR_STREAM("No formatter of type " << formatter_type << " has been implemented. Typo?");
            continue;
        }

        if (!cpp_outputters.count(outputter_type))
        {
            ROS_ERROR_STREAM("No outputter of type " << outputter_type << " has been implemented. Typo?");
            continue;
        }

        ros::param::set("/data_collectors" + ros::this_node::getName() + "/" + formatter_name + "_to_" + outputter_name, false);

        rios::data_collector::DataFormatter* formatter = cpp_formatters.at(formatter_type)(formatters_configured[formatter_name]);
        rios::data_collector::DataOutputter* outputter = cpp_outputters.at(outputter_type)(outputters_configured[outputter_name]);

        ingestor.registerStoreCallback(
            [formatter_name, outputter_name, formatter, outputter, &collection_config](std::deque<rios::data_collector::DxRosMsg::Ptr> data, std::string snapshot_name)
            {
                // Start a thread to do data formatting and output
                ROS_INFO_STREAM("[" << formatter_name << " -> " << outputter_name << "] Data store beginning for " << data.size() << " messages. Snapshot: " << snapshot_name);

                std::thread(
                    [formatter_name, outputter_name, formatter, outputter, &collection_config, data, snapshot_name]() mutable
                    {
                        // Format the data 
                        auto start = std::chrono::steady_clock::now();
                        rios::data_collector::FormattedData::Ptr formatted_data = formatter->formatData(data, snapshot_name);
                        auto end = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                        ROS_INFO_STREAM("[" << formatter_name << "] Formatting took " << elapsed << "ms"); 

                        // Output the data
                        start = std::chrono::steady_clock::now();
                        std::string error_str;
                        bool out_success = outputter->outputData(snapshot_name, formatted_data, error_str);
                        end = std::chrono::steady_clock::now();
                        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                        if (out_success)
                        {
                            ROS_INFO_STREAM("[" << outputter_name << "] Outputting succeeded in " << elapsed << "ms"); 
                        }
                        else
                        {
                            ROS_ERROR_STREAM("[" << outputter_name << "] Outputting failed in " << elapsed << "ms with error: " << error_str); 
                        }
                    }
                ).detach();
            }
        );

        ROS_INFO_STREAM("Successfully set up pipeline [ROS] -> [" << formatter_name << "] -> [" << outputter_name << "]");
        ros::param::set("/data_collectors" + ros::this_node::getName() + "/" + formatter_name + "_to_" + outputter_name, true);
        pipelines_setup++;
    }

    return pipelines_setup;
}

/**
 * @brief Main loop of the collector
 *
 * @param argc
 */
int main(int argc, char** argv)
{
    // Initializes ROS, and sets up a node
    ros::init(argc, argv, "dx_data_collector");
    ros::NodeHandle nh("~");

    // Start sysmon
    rios::sysmon::monitors::SysmonClient sysmon_client;
    ROS_INFO_STREAM("checking dependencies (Sysmon)...");

    if (!sysmon_client.waitDependencies())
    {
        ROS_ERROR_STREAM("shutting down process: dependencies down after timeout");
        return -1;
    }

    if (!sysmon_client.start())
    {
        ROS_ERROR_STREAM("shutting down process: Sysmon failed to start");
        return -1;
    }

    // Load the configuration from the param server
    rios::cfg collection_config = rios::yaml::getRosYaml(nh, "config");

    // Create the data ingestor
    rios::data_collector::RosDataIngestor ingestor(collection_config["ingestion"], collection_config["episode_name"].as<std::string>());

    // Create a map of configured formatters and outputters
    std::unordered_map<std::string, rios::cfg> formatters_configured;
    std::unordered_map<std::string, rios::cfg> outputters_configured;

    for (const auto & formatter_configured : collection_config["formatters"])
    {
        if (formatter_configured["name"])
        {
            formatters_configured[formatter_configured["name"].as<std::string>()] = formatter_configured.as<rios::cfg>();
        }
    }

    for (const auto & outputter_configured : collection_config["output"])
    {
        if (outputter_configured["name"])
        {
            outputters_configured[outputter_configured["name"].as<std::string>()] = outputter_configured.as<rios::cfg>();
        }
    }

    int pipelines_setup = setupPipelines(collection_config, ingestor, formatters_configured, outputters_configured);
    
    if (pipelines_setup)
    {
        ros::spin();
    }
    else
    {
        ROS_ERROR_STREAM("No pipelines were set up! Exiting the data collector.");
    }
}
