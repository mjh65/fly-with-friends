target_sources(fwf_lib_common PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/addrfinder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/clientlink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fwfsocket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/members.cpp
    ${CMAKE_CURRENT_LIST_DIR}/prefs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sessionhub.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sockcomms.cpp
)

if(WIN32)
    add_definitions(-DTARGET_WINDOWS)
    target_link_libraries(fwf_lib_common
        ws2_32
    )
elseif(APPLE)
    add_definitions(-DTARGET_MACOSX)
elseif(UNIX)
    add_definitions(-DTARGET_LINUX)
endif()

