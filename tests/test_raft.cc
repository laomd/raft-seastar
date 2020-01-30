#include "test_runner.hh"
#include <chrono>
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;
using namespace std::chrono;

SEASTAR_TEST_CASE(TestInitialElection2A) {
  size_t servers = 3;
  auto runner = seastar::make_shared<TestRunner>(servers, 100ms, 10ms);
  return runner->start_servers()
      .then([runner] { return runner->checkOneLeader(); })
      .finally([runner] {});
}
