file(GLOB TEST_SRCS test_*.cc)
foreach(case ${TEST_SRCS})
  get_filename_component(case_name ${case} NAME_WE)
  seastar_add_test(${case_name}
    SOURCES ${case}
    LIBRARIES raft
    DEPENDENCIES raft_server)
endforeach()
