#include "test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(InitialElection2A) {
  return env->checkOneLeader().discard_result();
}
