set(RAFT_SRCS service/raft_impl.cc service/logentry_applier.cc)

add_library(raft ${RAFT_SRCS})
link_directories(${CMAKE_BINARY_DIR}/rpc)
target_link_libraries(raft PUBLIC rpc)

add_executable(raft_server server.cc)
target_link_libraries(raft_server raft)
