cmake_minimum_required(VERSION 3.11)
project(lao_hdfs VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(SMF_MANAGE_DEPS OFF CACHE "" INTERNAL FORCE)
set(SMF_BUILD_PROGRAMS OFF CACHE "" INTERNAL FORCE)
set(SMF_ENABLE_TESTS OFF CACHE "" INTERNAL FORCE)
add_subdirectory(smf EXCLUDE_FROM_ALL)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(util)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_libraries(smf::smf)

set(CMAKE_CXX_STANDARD 20)
enable_testing()

add_subdirectory(raft)
add_subdirectory(tests)
