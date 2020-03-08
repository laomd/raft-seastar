set(RAFT_SRCS service/raft_impl.cc)

add_library(raft ${RAFT_SRCS})
link_directories(${CMAKE_BINARY_DIR}/rpc)
target_link_libraries(raft PUBLIC rpc)
