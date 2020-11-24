# X-Plane plugin

add_library(fwf_plugin SHARED
    ${CMAKE_CURRENT_LIST_DIR}/xplmain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/uimanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fwfdialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/hostdialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/joindialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/statusdialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/simaccess.cpp
)

add_definitions(-DXPLM303=1 -DXPLM302=1 -DXPLM301=1 -DXPLM300=1 -DXPLM210=1 -DXPLM200=1)

if(WIN32)
    add_definitions(-DTARGET_WINDOWS -DIBM=1)
    target_link_libraries(fwf_plugin
        -static
        -static-libgcc
        -static-libstdc++
        fwf_lib_common
        ${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Win/XPLM_64.lib
        ${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Win/XPWidgets_64.lib
        opengl32
    )
elseif(APPLE)
    add_definitions(-DTARGET_MACOSX -DAPL=1)
    include_directories("/usr/local/include")
    link_directories("/usr/local/lib")
    target_link_libraries(fwf_plugin
        fwf_lib_common
        "-F${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Mac -framework XPLM"
        "-F${PROJECT_SOURCE_DIR}/lib/XSDK/Libraries/Mac -framework XPWidgets"
        "-framework OpenGL"
    )
elseif(UNIX)
    add_definitions(-DTARGET_LINUX -DLIN=1)
    target_link_libraries(fwf_plugin
        -static-libgcc
        -static-libstdc++
        -fPIC
        fwf_lib_common
    )
endif()

set_target_properties(fwf_plugin PROPERTIES PREFIX "")
set_target_properties(fwf_plugin PROPERTIES OUTPUT_NAME "FlyWithFriends")
set_target_properties(fwf_plugin PROPERTIES SUFFIX ".xpl")

