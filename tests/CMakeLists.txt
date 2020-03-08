include(test)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(state_machine)

add_executable(raft_server raft/server.cc)
target_link_libraries(raft_server raft state_machine)

file(GLOB TEST_SRCS test_*.cc)
foreach(case ${TEST_SRCS})
  get_filename_component(case_name ${case} NAME_WE)
  seastar_add_test(${case_name}
    SOURCES ${case}
    LIBRARIES raft state_machine
    DEPENDENCIES raft_server)
endforeach()
