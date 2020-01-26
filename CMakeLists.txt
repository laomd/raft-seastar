cmake_minimum_required(VERSION 3.11)
project(lao_hdfs VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(ENV{PKG_CONFIG_PATH} "$PKG_CONFIG_PATH:/usr/lib64/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig")

find_package(PkgConfig REQUIRED)
pkg_search_module(SEASTAR REQUIRED seastar)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_libraries(${SEASTAR_STATIC_LDFLAGS})
add_compile_options(
  ${SEASTAR_STATIC_CFLAGS_OTHER}
)

add_subdirectory(raft)
