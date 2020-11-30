set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
if(APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

add_library(fwf_lib_common)

include(${CMAKE_CURRENT_LIST_DIR}/common/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/server/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/plugin/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/robot/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/testapp/CMakeLists.txt)

include_directories("${CMAKE_CURRENT_LIST_DIR}/include")

