# CMake file for the Fly-with-Friends server application

cmake_minimum_required(VERSION 3.12)
project(FWF-Server VERSION 1.0 DESCRIPTION "Fly-with-Friends server application")

add_executable(fwfserver)

if(WIN32)
    add_compile_definitions(TARGET_WINDOWS)
elseif(APPLE)
    add_compile_definitions(TARGET_MACOSX)
elseif(UNIX)
    add_compile_definitions(TARGET_LINUX)
endif()

target_sources(fwfserver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/servermain.cpp
)

target_link_libraries(fwfserver PUBLIC fwf_compiler_flags fwfengine)
