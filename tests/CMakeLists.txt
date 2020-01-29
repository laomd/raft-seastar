include(test)

seastar_add_test(raft 
  SOURCES test_raft.cc
  LIBRARIES raft
  DEPENDENCIES raft_server)
