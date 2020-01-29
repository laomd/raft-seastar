cmake_minimum_required(VERSION 3.11)
project(raft VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(Seastar_TESTING 1)
set(Seastar_STD_OPTIONAL_VARIANT_STRINGVIEW 1)
set(Seastar_EXPERIMENTAL_COROUTINES_TS 1)
add_compile_options(-Wno-deprecated-declarations)

set(Seastar_EXPERIMENTAL_COROUTINES_TS 1)
add_subdirectory(seastar)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/seastar/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_libraries(Seastar::seastar)

add_subdirectory(raft)
enable_testing()
add_subdirectory(tests)
