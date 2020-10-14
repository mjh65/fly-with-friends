include_directories("${CMAKE_CURRENT_LIST_DIR}/XSDK/CHeaders")

add_library(lipify STATIC "${CMAKE_CURRENT_LIST_DIR}/lipify/ipify.c")
target_compile_definitions(lipify PRIVATE PACKAGE_NAME="lipify" PACKAGE_VERSION="0.5")
include_directories("${CMAKE_CURRENT_LIST_DIR}/lipify")
