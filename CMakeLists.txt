cmake_minimum_required(VERSION 3.11)
project(raft VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_compile_options(-Wno-deprecated-declarations)
add_subdirectory(seastar)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/seastar/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_libraries(Seastar::seastar)

find_package (Boost REQUIRED
  COMPONENTS unit_test_framework)
add_library (raft_testing
  seastar/include/seastar/testing/entry_point.hh
  seastar/include/seastar/testing/exchanger.hh
  seastar/include/seastar/testing/seastar_test.hh
  seastar/include/seastar/testing/test_case.hh
  seastar/include/seastar/testing/test_runner.hh
  seastar/include/seastar/testing/thread_test_case.hh
  seastar/src/testing/entry_point.cc
  seastar/src/testing/seastar_test.cc
  seastar/src/testing/test_runner.cc)

target_compile_definitions (raft_testing
  PUBLIC BOOST_TEST_DYN_LINK
  PRIVATE ${Seastar_PRIVATE_COMPILE_DEFINITIONS})

target_compile_options (raft_testing
  PRIVATE ${Seastar_PRIVATE_CXX_FLAGS})

target_link_libraries (raft_testing
  PUBLIC
  Boost::unit_test_framework)


add_subdirectory(raft)
enable_testing()
add_subdirectory(tests)
