cmake_minimum_required(VERSION 3.12)
project(dx_data_collector_commander)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##############################################################

option(
  
  PYTHON_BINDINGS "enable and compile python bindings in this project" ON

)

find_package(
  
  catkin REQUIRED COMPONENTS 
    roscpp
    dx_data_collector_msgs
    dx_rios_utils
    dx_rios_pybind

)

catkin_package(
  
  INCLUDE_DIRS
    include
    include/dx_data_collector_commander
  
  CATKIN_DEPENDS
    dx_data_collector_msgs
    dx_rios_utils
    dx_rios_pybind

)

include_directories(
  
  include
  include/dx_data_collector_commander
  ${catkin_INCLUDE_DIRS}

)

add_library(${PROJECT_NAME}
  src/dx_data_collector_commander/data_collector_commander.cpp
)

target_link_libraries(${PROJECT_NAME}
  ${catkin_LIBRARIES}
)

install(
  
  DIRECTORY include/ include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"

)

install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

if (PYTHON_BINDINGS)

  add_subdirectory(bindings/python/)

endif (PYTHON_BINDINGS)
