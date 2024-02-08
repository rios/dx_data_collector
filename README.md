# dx_data_collector
RIOS data

## Overview ##
`dx_data_collector` is responsbile for collecting and exporting data from `ROS` topics.

## Configuration ##
See [here](dx_data_collector/config/data_collection_config.yaml) for an example configuration.

Essentially you need to:
- Configure one or more topics to listen to
- Configure one or more `formatter`s that will organize and format the data when outputting is triggered
- Configure one or more `outputter`s that will output the formatted data somewhere
- Set up one or more collection `pipeline`s consisting of a `formatter` and an `outputter` each.

The config file examples are the best place to see available formatters and outputters as well as their configurations.

## Runtime Use ##
Under the hood, the data collector maintains a buffer of data (in a RIOS internal format defined in [dx_data_collector/include/data_types/dx_ros_msg.h](dx_data_collector/include/data_types/dx_ros_msg.h) for the last bit of time, defined by the `buffer_length_s` config variable. The only function possible at runtime is to output some subset of this data. The `dx_data_collector_commander` allows "recording" but in effect it's just recording the timestamps for later output. The `buffer_length_s` puts an upper bound on the amount of data that can be output at one time.

The dev ui allows recording from a webpage and otherwise, it should be triggered by the `dx_data_collector`.

## Design ##
### Overview ###
The major elements of the architecture are `Ingestion`, `Formatters`, and `Outputters`. Ingestion takes in data and stores it in a buffer. When requested, it starts the output pipeline. `Formatters` format the internal data coming from ingestion and `Outputters` output the formatted data. The pipelines can then be made by mixing and matching formatters and ingestors.

### Ingestion ###
The ingestion implementation is in [dx_data_collector/src/ros_ingestion.cpp](dx_data_collector/src/ros_ingestion.cpp). There is a global data buffer `RosDataBuffer` that stores a moving window of data for a configurable period of time. The buffer is written to by individual `TopicIngestor` objects instantiated for each configured topic. The topic ingestors can make subscriptions to any topic without knowing its type. This uses the `ros_msg_parser` code initially taken from [here](https://github.com/facontidavide/ros_msg_parser). Each message's data is stored in the buffer as raw binary until it needs to be outputted. Outputting is triggered by a service that defines the start and end time for the data of interest. At this time, a queue of pointers to the messages representing this data is built up from the buffer and sent down the pipeline.

### Formatting ###
A `formatter` is an implementation of the interface defined in [dx_data_collector/include/interfaces/data_formatter.h](dx_data_collector/include/interfaces/data_formatter.h). It takes as input a queue of shared pointers to `DxRosMsg` objects (thereby keeping them alive even if they're not in the buffer) and it is able to call `DxRosMsg` functions to parse the binary data or do whatever else may need to be done to format the data. The output is a data object which implements the interface defined in the same file as the data formatter definition. The only requirement is that it be able to output its formatted data to a filesystem path. Formatters must be added to the `cpp_outputters` map in [dx_data_collector/src/dx_data_collector_node.cpp](dx_data_collector/src/dx_data_collector_node.cpp).

### Outputting ###
An `outputter` is an implementation of the interface defined in [dx_data_collector/include/interfaces/data_outputter.h](dx_data_collector/include/interfaces/data_outputter.h). It takes as input a pointer to some formatted data and must implement a function to output the data however appropriate. Outputters must be added to the `cpp_outputters` map in [dx_data_collector/src/dx_data_collector_node.cpp](dx_data_collector/src/dx_data_collector_node.cpp).