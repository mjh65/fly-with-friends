cmake_minimum_required(VERSION 3.9)
project(FlyWithFriends VERSION 0.0.1 DESCRIPTION "X-Plane plugin for simple group flying")

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

add_definitions("-include ${PROJECT_BINARY_DIR}/config.h")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

include(lib/CMakeLists.txt)
include(src/CMakeLists.txt)
