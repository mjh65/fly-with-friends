set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fPIC")

add_library(fwf_lib_common)
add_library(fwf_lib_plugin)
add_library(fwf_lib_testapp)

add_definitions(-DXPLM303=1 -DXPLM302=1 -DXPLM301=1 -DXPLM300=1 -DXPLM210=1 -DXPLM200=1)

include(${CMAKE_CURRENT_LIST_DIR}/common/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/plugin/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/testapp/CMakeLists.txt)

include_directories(
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/inc")

if(WIN32)
    target_link_libraries(fwf_lib_common
        ws2_32
    )
endif(WIN32)

# X-Plane plugin
add_library(fwf_plugin SHARED
    ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
)

if(WIN32)
    add_definitions(-DIBM=1)
    target_link_libraries(fwf_plugin
        -static
        -static-libgcc
        -static-libstdc++
        fwf_lib_common
        fwf_lib_plugin
        lipify
        ${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Win/XPLM_64.lib
        ${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Win/XPWidgets_64.lib
        opengl32
    )
elseif(APPLE)
    add_definitions(-DAPL=1)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
    target_link_libraries(fwf_plugin
        fwf_lib_common
        fwf_lib_plugin
        lipify
        "-F${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Mac -framework XPLM"
        "-framework OpenGL"
    )
elseif(UNIX)
    add_definitions(-DLIN=1)
    target_link_libraries(fwf_plugin
        -static-libgcc
        -static-libstdc++
        -fPIC
        fwf_lib_common
        fwf_lib_plugin
        lipify
    )
endif()

set_target_properties(fwf_plugin PROPERTIES PREFIX "")
set_target_properties(fwf_plugin PROPERTIES OUTPUT_NAME "FlyWithFriends")
set_target_properties(fwf_plugin PROPERTIES SUFFIX ".xpl")

# Standalone test program
add_executable(fwf_testapp
    ${CMAKE_CURRENT_LIST_DIR}/testapp.cpp
)

if(WIN32)
    target_link_libraries(fwf_testapp
        -static
        -static-libgcc
        -static-libstdc++
        fwf_lib_common
        fwf_lib_testapp
        lipify
        -Wl,--subsystem,windows
    )
elseif(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
    target_link_libraries(fwf_testapp
        fwf_lib_common
        fwf_lib_testapp
        lipify
    )
elseif(UNIX)
    target_link_libraries(fwf_testapp
        fwf_lib_common
        fwf_lib_testapp
        lipify
    )
endif()
