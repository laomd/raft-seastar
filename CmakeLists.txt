cmake_minimum_required(VERSION 3.11)
project(lao_hdfs VERSION 0.1)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH /usr/local/lib64/cmake/smf/)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(ENV{PKG_CONFIG_PATH} "$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig")
include(smfc_generator)

find_package(PkgConfig REQUIRED)
find_package(smf REQUIRED)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR})

include(util)
link_libraries(smf::smf)

enable_testing()
include(test)
add_subdirectory(raft)
