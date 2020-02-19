set(RAFT_SRCS service/raft_impl.cc service/logentry_applier.cc)

add_library(raft ${RAFT_SRCS})
add_library(log_service service/logentry_applier.cc)
link_directories(${CMAKE_BINARY_DIR}/rpc)
link_libraries(rpc)

add_executable(raft_server server.cc)
target_link_libraries(raft_server raft)

add_executable(log_server log_server.cc)
target_link_libraries(log_server log_service)