# Third-Party Components

This repository vendors the following third-party code. The root Apache-2.0 license
does not extend to these files; each is governed by the license shown.

## ros_msg_parser

- **Location:** `dx_data_collector/include/ros_msg_parser/` (14 files)
- **Upstream:** https://github.com/facontidavide/ros_msg_parser
- **License:** MIT
- **Copyright:** (c) 2016-2022 Davide Faconti
- **Purpose:** Runtime introspection and deserialization of ROS messages without
  compile-time type knowledge, used to parse recorded bag data.

The MIT license text is retained in the header of each vendored file.

## span-lite

- **Location:** `dx_data_collector/include/ros_msg_parser/utils/span.hpp`
- **Upstream:** https://github.com/martinmoene/span-lite
- **License:** Boost Software License 1.0
- **Copyright:** 2018-2019 Martin Moene
- **Purpose:** `std::span` backport for pre-C++20 compilers; a dependency of
  `ros_msg_parser`, vendored alongside it.

The Boost Software License notice is retained in the file header.
