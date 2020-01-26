smfc_gen(
  CPP
  TARGET_NAME service
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES common.fbs service.fbs)
add_library(raft raft_impl.cc "${service}")
link_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(raft_server server.cc)
target_link_libraries(raft_server raft)