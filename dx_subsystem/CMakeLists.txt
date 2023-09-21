cmake_minimum_required(VERSION 3.0.2)
project(dx_subsystem)

add_compile_options(-std=c++17)

find_package(
  catkin REQUIRED
  COMPONENTS 
    dx_subsystem_msgs
    roscpp
    rospy
    std_msgs
    dx_sysmon_sdk
)

add_executable(dx_subsystem_node src/dx_subsystem_node.cpp)

target_link_libraries(
  dx_subsystem_node ${catkin_LIBRARIES}
)

catkin_python_setup()


catkin_package(

  CATKIN_DEPENDS  rospy std_msgs

)

include_directories(
  include 
  include/dx_subsystem
  ${catkin_INCLUDE_DIRS}
)

catkin_install_python(PROGRAMS nodes/subsystem_node.py DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

install(DIRECTORY launch nodes config DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS dx_subsystem_node RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})