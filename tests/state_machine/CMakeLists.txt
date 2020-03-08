add_library(state_machine kv_state_machine.cc)
link_directories(${CMAKE_BINARY_DIR}/rpc)
target_link_libraries(state_machine PUBLIC rpc)
