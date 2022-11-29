# Top-level CMake file for Fly-with-Friends (FWF)
#
# FWF is primarily an X-Plane plugin, built around an engine which can be imported
# into other projects for custom use. There are also standalone executables for a
# server application and a robot application that can replay recorded flights.

cmake_minimum_required(VERSION 3.12)
project(FlyWithFriends VERSION 1.0 DESCRIPTION "Fly-with-Friends X-Plane plugin")

add_library(fwf_compiler_flags INTERFACE)
target_compile_features(fwf_compiler_flags INTERFACE cxx_std_17)
if(WIN32)
    target_link_libraries(fwf_compiler_flags INTERFACE wsock32 ws2_32)
endif()

add_subdirectory("engine")
add_subdirectory("robot")
add_subdirectory("server")
