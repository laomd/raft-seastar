cmake_minimum_required(VERSION 3.11)
project(raft VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(Seastar_DPDK ON)
option(Seastar_APPS OFF)
option(Seastar_DOCS OFF)
option(Seastar_DEMOS OFF)
option(Seastar_STD_OPTIONAL_VARIANT_STRINGVIEW ON)
set(BUILD_SHARED_LIBS ON)
# Variadic future<> with more than one template parmeter is deprecated, replace with future<std::tuple<...>> 
add_compile_options(-Wno-deprecated-declarations -fPIC)
add_subdirectory(seastar)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/seastar/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_libraries(Seastar::seastar)

add_subdirectory(raft)
