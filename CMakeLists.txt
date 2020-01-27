cmake_minimum_required(VERSION 3.11)
project(raft VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(Seastar_DPDK OFF)
option(Seastar_APPS OFF)
option(Seastar_DEMO OFF)
option(Seastar_DOCS OFF)
option(Seastar_TESTING OFF)
option(Seastar_STD_OPTIONAL_VARIANT_STRINGVIEW ON)
add_compile_options(-Wno-deprecated-declarations)
add_subdirectory(seastar)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/seastar/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_libraries(Seastar::seastar)

add_subdirectory(raft)
