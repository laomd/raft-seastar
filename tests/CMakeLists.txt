include(test)

seastar_add_test(raft
  SOURCES raft/test_raft.cc
  LIBRARIES raft
)