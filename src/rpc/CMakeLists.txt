add_library(rpc rpc.cc)
target_compile_options(rpc PRIVATE -Wno-reorder)
target_link_libraries(rpc PUBLIC Seastar::seastar)
link_directories(${CMAKE_CURRENT_BINARY_DIR})