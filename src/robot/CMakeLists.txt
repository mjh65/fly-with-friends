# Standalone test program
add_executable(fwf_robot
    ${CMAKE_CURRENT_LIST_DIR}/robotmain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/flightreplay.cpp
)

set_target_properties(fwf_robot PROPERTIES PREFIX "")
set_target_properties(fwf_robot PROPERTIES OUTPUT_NAME "FWFRobot")

if(WIN32)
    add_definitions(-DTARGET_WINDOWS)
    target_link_libraries(fwf_robot
        -static
        -static-libgcc
        -static-libstdc++
        fwf_lib_common
        -Wl,--subsystem,windows
    )
    set_target_properties(fwf_robot PROPERTIES SUFFIX ".exe")
elseif(APPLE)
    add_definitions(-DTARGET_MACOSX)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
    target_link_libraries(fwf_robot
        fwf_lib_common
    )
    set_target_properties(fwf_robot PROPERTIES SUFFIX "")
elseif(UNIX)
    add_definitions(-DTARGET_LINUX)
    target_link_libraries(fwf_robot
        fwf_lib_common
    )
    set_target_properties(fwf_robot PROPERTIES SUFFIX "")
endif()

