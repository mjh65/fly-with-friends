# CMake file for the Fly-with-Friends engine library

cmake_minimum_required(VERSION 3.12)
project(FWF-Engine-Lib VERSION 1.0 DESCRIPTION "Fly-with-Friends engine library")

add_library(fwfengine STATIC)

if(WIN32)
    add_compile_definitions(TARGET_WINDOWS)
elseif(APPLE)
    add_compile_definitions(TARGET_MACOSX)
elseif(UNIX)
    add_compile_definitions(TARGET_LINUX)
endif()

target_sources(fwfengine PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/addrfinder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/aircraft.cpp
    ${CMAKE_CURRENT_LIST_DIR}/aircraftdb.cpp
    ${CMAKE_CURRENT_LIST_DIR}/clientlink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/datagrams.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fwfsocket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sessionhub.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sockcomms.cpp
)

target_include_directories(fwfengine INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
