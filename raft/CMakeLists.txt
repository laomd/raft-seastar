smfc_gen(
  CPP
  TARGET_NAME service
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES common.fbs service.fbs)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${CMAKE_CURRENT_BINARY_DIR})
# add_library(raft raft_impl.cc "${service}")

add_executable(test_raft test/test_raft.cc raft_impl.cc "${service}")
add_test(
  NAME raft
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/start.sh
)