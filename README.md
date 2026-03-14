# dx_data_collector

> **ARCHIVED REPOSITORY**
> This repository has been archived. It is published here for transparency and reference.

A generic, configurable data collection pipeline for [ROS (Robot Operating System)](https://www.ros.org/).
`dx_data_collector` subscribes to ROS topics, buffers incoming messages in a rolling time window, and
exports data on demand through a pipeline of pluggable formatters and outputters.

## Packages

| Package | Description |
|---|---|
| `dx_data_collector` | Core data collection node: ingestion, formatting, and output |
| `dx_data_collector_commander` | Client interface for triggering data collection recordings |
| `dx_data_collector_msgs` | ROS message and service definitions |

## Overview

`dx_data_collector` maintains a rolling in-memory buffer of ROS topic data for a configurable
time window (`buffer_length_s`). When triggered, it runs the buffered data through one or more
**pipelines**, each consisting of a **formatter** and an **outputter**. This allows the same data
to be simultaneously written as a rosbag to the filesystem and as JSON to S3-compatible storage,
for example.

## Architecture

The three major components are:

- **Ingestion** (`ros_ingestion.cpp`): Subscribes to any ROS topic without knowing its type in
  advance, using the bundled `ros_msg_parser` library (originally from
  [facontidavide/ros_msg_parser](https://github.com/facontidavide/ros_msg_parser)).
  Raw binary messages are held in the `RosDataBuffer` circular buffer.

- **Formatters**: Implementations of `DataFormatter` that consume the buffer and produce structured
  output. Built-in formatters: `json`, `rosbag`, `ia` (video/frames), `dummy`.

- **Outputters**: Implementations of `DataOutputter` that write formatted data to a destination.
  Built-in outputters: `filesystem`, `s3` (S3-compatible object storage), `dummy`.

## Configuration

See [`dx_data_collector/config/data_collection_config.yaml`](dx_data_collector/config/data_collection_config.yaml)
for a full example. A minimal configuration looks like:

```yaml
episode_name: my_dataset
ingestion:
  buffer_length_s: 60
  topics:
    - /camera/image_raw
    - /tf

formatters:
  - name: bag
    type: rosbag

output:
  - name: local
    type: filesystem
    path: /tmp/dx_data_collector

pipelines:
  - formatter: bag
    outputter: local
```

For S3/object storage output, set the environment variables listed in [`.env.example`](.env.example)
and configure the `s3` outputter type.

## Requirements

- ROS Noetic (Ubuntu 20.04)
- C++17
- [AWS SDK for C++](https://github.com/aws/aws-sdk-cpp) (required only if using the S3 outputter)
- Catkin build system

Internal build dependencies (`dx_rios_yaml`, `dx_rios_utils`, `dx_sysmon_sdk`, `dx_rios_pybind`)
were RIOS-internal packages. See [docker/submodules.repos](docker/submodules.repos) for the
original repository locations — these are not publicly available.

## Building

This project uses [Earthly](https://earthly.dev) for containerized builds. See `docker/Earthfile`.
For a local catkin build:

```bash
cd <catkin_ws>/src
# Place packages here or add to workspace
catkin build dx_data_collector
```

## Running

```bash
roslaunch dx_data_collector dx_data_collector.launch \
  data_collection_config:=/path/to/your/config.yaml
```

## Environment Variables

See [`.env.example`](.env.example) for all configurable environment variables, particularly
those required for S3 output.

## License

Apache-2.0. See [LICENSE](LICENSE).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
