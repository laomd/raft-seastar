add_library(raft raft_impl.cc)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(raft_server server.cc)
target_link_libraries(raft_server raft)