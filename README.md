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

## Prerequisites & Known Limitations

This repository is **archived** and was extracted from RIOS AI's internal monorepo. It will not build out-of-the-box without substituting several internal dependencies. The notes below describe what is required and what needs replacement.

### Platform / Hardware

- **ROS 1 Noetic only.** The package uses Catkin, `roscpp`, and the rosbag1 format. There is no ROS 2 port.
- **Ubuntu 20.04** is the only tested host OS (this is the Noetic-supported pair).
- **C++17** toolchain required.
- No GPU is required by the core node. The `ia` formatter handles video/frames but does not itself depend on CUDA.

### Internal RIOS Dependencies (Not on Public PyPI / GitHub)

The following packages were hosted on RIOS-internal infrastructure (`<internal_pypi>` and private GitHub orgs) and are **not publicly available**. Builds that pull them via `docker/submodules.repos` will fail:

- `dx_rios_yaml` — internal YAML config loader used by the data collector node
- `dx_rios_utils` — internal C++/Python utility library
- `dx_sysmon_sdk` — internal system-monitoring SDK
- `dx_rios_pybind` — internal pybind11 helpers (used by `dx_data_collector_commander/bindings`)

To build this repo standalone, you will need to either stub out, vendor, or replace the call sites that use these libraries. They are referenced in `CMakeLists.txt`, `package.xml`, and `submodules.repos`.

### Build System

- The provided `docker/Earthfile` uses [Earthly](https://earthly.dev) and imports `<internal_build_framework>` and `<internal_build_framework>/ros1`. **These repositories are private** and the `RIOS+rios-ros` base image cannot be pulled externally. The Earthfile is included for reference only — external users should fall back to a plain `catkin build` against a stock `ros:noetic-ros-base` image.
- `docker/submodules.repos` points to private RIOS git repositories; `vcs import` against it will fail without VPN/credential access that no longer exists.

### Outputters / External Services

- The `s3` outputter requires the [AWS SDK for C++](https://github.com/aws/aws-sdk-cpp) installed at build time. If you do not need S3 output, you can remove the `s3` source files and the dependency.
- S3 endpoints, buckets, and credentials are configured via environment variables (see `.env.example`). The original deployments targeted RIOS-internal MinIO/S3-compatible endpoints; you will need to substitute your own.

### Maintenance Status

This repository is **archival and unmaintained**. No issues, PRs, or security updates will be accepted. It is published for reference and transparency only.
