cmake_minimum_required(VERSION 3.0.2)
project(dx_data_collector_msgs)

find_package(catkin REQUIRED message_generation std_msgs)

# Generate messages in the 'msg' folder
add_message_files(
  FILES
  SystemState.msg
)

## Generate services in the 'srv' folder
add_service_files(
  FILES
  Snapshot.srv
  GetTime.srv
)

# Generate added messages and services with any dependencies listed here
generate_messages(DEPENDENCIES std_msgs)

catkin_package(
  CATKIN_DEPENDS message_runtime std_msgs
)
