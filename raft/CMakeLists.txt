smfc_gen(
  CPP
  TARGET_NAME service
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES common.fbs service.fbs)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${CMAKE_CURRENT_BINARY_DIR})
# add_library(raft raft_impl.cc "${service}")

seastar_add_test(raft2
  SOURCES test/test_raft.cc raft_impl.cc "${service}"
)