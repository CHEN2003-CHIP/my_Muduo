cmake_minimum_required(VERSION 2.5)
project(mymuduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

aux_source_directory(. SRC_LIST)

add_library(mymuduo  SHARED ${SRC_LIST})
target_link_libraries(mymuduo pthread)

option(MYMUDUO_BUILD_EXAMPLES "Build mymuduo examples" OFF)
if(MYMUDUO_BUILD_EXAMPLES)
    include_directories(${PROJECT_SOURCE_DIR})
    add_executable(testserver examples/testserver.cc)
    target_link_libraries(testserver mymuduo pthread)
endif()
