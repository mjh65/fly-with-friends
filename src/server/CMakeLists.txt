# Standalone server executable (console application)

add_executable(fwf_server
    ${CMAKE_CURRENT_LIST_DIR}/servermain.cpp
)

include_directories("${CMAKE_CURRENT_LIST_DIR}/../common")

set_target_properties(fwf_server PROPERTIES PREFIX "")
set_target_properties(fwf_server PROPERTIES OUTPUT_NAME "FWFServer")

if(WIN32)
    add_definitions(-DTARGET_WINDOWS)
    target_link_libraries(fwf_server
        -static
        -static-libgcc
        -static-libstdc++
        fwf_lib_common
        -Wl,--subsystem,windows
    )
    set_target_properties(fwf_server PROPERTIES SUFFIX ".exe")
elseif(APPLE)
    add_definitions(-DTARGET_MACOSX)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
    target_link_libraries(fwf_server
        fwf_lib_common
    )
    set_target_properties(fwf_server PROPERTIES SUFFIX "")
elseif(UNIX)
    add_definitions(-DTARGET_LINUX)
    target_link_libraries(fwf_server
        fwf_lib_common
    )
    set_target_properties(fwf_server PROPERTIES SUFFIX "")
endif()

