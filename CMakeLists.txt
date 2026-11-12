# EH3D-NODE-HMI-GUI - the ESP-IDF project of the touch screen (ESP32-P4). GPL-3.0-or-later.
cmake_minimum_required(VERSION 3.16)
# The firmware's version is the project's: it is read from eh3d.project.json, the one place that says it.
file(READ "${CMAKE_CURRENT_LIST_DIR}/eh3d.project.json" EH3D_MANIFEST)
string(REGEX MATCH "\"version\"[ 	]*:[ 	]*\"([0-9]+\.[0-9]+\.[0-9]+)\"" EH3D_VERSION_MATCH "${EH3D_MANIFEST}")
set(PROJECT_VER "${CMAKE_MATCH_1}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/eh3d.project.json")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# EH3D_STACK_REPORT=1 (the build in GitHub Actions sets it) makes the compiler write the stack use of every function (.su files) so that the build can list the biggest ones; the image does not change.
if(DEFINED ENV{EH3D_STACK_REPORT})
  idf_build_set_property(COMPILE_OPTIONS "-fstack-usage" APPEND)
endif()
project(eh3d-node-hmi-gui)
