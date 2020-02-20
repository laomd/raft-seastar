#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(InitialElection2A) {
  return with_env([](auto &env, int, ms_t) {
    return env.checkOneLeader().discard_result();
  });
}
