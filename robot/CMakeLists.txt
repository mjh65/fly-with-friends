# CMake file for the Fly-with-Friends robot application

cmake_minimum_required(VERSION 3.12)
project(FWF-Robot VERSION 1.0 DESCRIPTION "Fly-with-Friends robot application")

add_executable(fwfrobot)

if(WIN32)
    add_compile_definitions(TARGET_WINDOWS)
elseif(APPLE)
    add_compile_definitions(TARGET_MACOSX)
elseif(UNIX)
    add_compile_definitions(TARGET_LINUX)
endif()

target_sources(fwfrobot PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/robotmain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/flightreplay.cpp
)

target_link_libraries(fwfrobot PUBLIC fwf_compiler_flags fwfengine)
