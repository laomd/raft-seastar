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
      .then([runner] { return runner->checkOneLeader().discard_result(); })
      .finally([runner] { return runner->clean_up(); });
}

SEASTAR_TEST_CASE(TestReElection2A) {
  size_t servers = 3;
  auto runner = seastar::make_shared<TestRunner>(servers, 100ms, 10ms);
  return runner->start_servers()
      .then([runner] { return runner->checkOneLeader(); })
      .then([runner](laomd::raft::id_t leader) {
        std::cout << "kill server " << leader << std::endl;
        return runner->kill(leader).then([runner, leader] {
          return runner->checkOneLeader().then([leader](auto new_leader) {
            BOOST_REQUIRE_NE(leader, new_leader);
            return leader;
          });
        });
      })
      .then([runner](laomd::raft::id_t leader) {
        std::cout << "restart server " << leader << std::endl;
        return runner->restart(leader).then(
            [runner] { return runner->checkOneLeader().discard_result(); });
      })
      .finally([runner] { return runner->clean_up(); });
}
