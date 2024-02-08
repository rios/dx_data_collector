cmake_minimum_required(VERSION 3.12.0)

set(

  PYBIND11_PYTHON_VERSION "3.8"

)

# Create a shortcut for the target name
set(
  
  module_name data_collector_commander

)

find_package(
  
  Python 3 REQUIRED COMPONENTS
    Development
    Interpreter

)

find_package(
  
  pybind11 REQUIRED

)

include_directories(

  ${catkin_INCLUDE_DIRS}

)

pybind11_add_module(

  ${module_name} commander.cpp

)

# linking the modules:

target_link_libraries(
  
  ${module_name} PRIVATE
    ${PROJECT_NAME}
    ${catkin_LIBRARIES}

)

# installing the modules:

install(

  TARGETS ${module_name}
  DESTINATION ${CATKIN_PACKAGE_PYTHON_DESTINATION}

)

# Include FetchContent CMake module
# ref: https://cmake.org/cmake/help/latest/module/FetchContent.html
include(FetchContent)

# Declare "pybind11stubgen" to be downloaded from the git repository below
FetchContent_Declare(

  pybind11stubgen
  GIT_REPOSITORY https://github.com/sizmailov/pybind11-stubgen.git
  GIT_TAG 2536dbc3cad276c9edea86a3af61de9a57ab644d # git commit hash for v0.10.5

)

# Make "pybind11stubgen" available, i.e. create the necessary CMake variables
FetchContent_MakeAvailable(pybind11stubgen)

# Create a convenience variable pointing to the stub generator command
set(stubgen_py ${pybind11stubgen_SOURCE_DIR}/pybind11_stubgen/__init__.py --ignore-invalid signature)

# Add a custom command and attach it to the post build stage of the target
add_custom_command(

  TARGET ${module_name}
  POST_BUILD  # attach to the post build stage
  #COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${Python3_EXECUTABLE} ${stubgen_py} ${PROJECT_NAME}  # run this command, e.g. PYTHONPATH=../../../ python3 __init__.py pibynd
  # IMPORTANT NOTE: Here is the argument list for the pybind11-stubgen
  # pybind11-stubgen [-h] [-o OUTPUT_DIR]
  #                  [--root-module-suffix ROOT_MODULE_SUFFIX]
  #                  [--no-setup-py]
  #                  [--ignore-invalid {signature,defaultarg,all} [{signature,defaultarg,all} ...]]
  #                  [--skip-signature-downgrade]
  #                  [--bare-numpy-ndarray]
  #                  [--log-level LOG_LEVEL]
  #                  MODULE_NAME [MODULE_NAME ...]
  # For instance, it is possible to disable setup.py generation like this
  #COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${Python3_EXECUTABLE} ${stubgen_py} --no-setup-py encoders
  # Command below works for a catkin project
  COMMAND PYTHONPATH=${CATKIN_DEVEL_PREFIX}/lib ${PYTHON_EXECUTABLE} ${stubgen_py} --no-setup-py ${module_name}
  COMMENT "Generate pybind11 stubs"
  VERBATIM  # make the command output verbose

)

# Install generated stubs
install(

  FILES ${CMAKE_CURRENT_BINARY_DIR}/stubs/${module_name}-stubs/__init__.pyi
  DESTINATION ${CATKIN_PACKAGE_PYTHON_DESTINATION}/${module_name}

)
