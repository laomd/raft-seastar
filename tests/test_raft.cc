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
        return runner->kill(leader).then([runner, leader] {
          // if the leader disconnects, a new one should be elected.
          return runner->checkOneLeader().then([leader](auto new_leader) {
            BOOST_REQUIRE_NE(leader, new_leader);
            return leader;
          });
        });
      })
      .then([runner](laomd::raft::id_t leader) {
        // if the old leader rejoins, that shouldn't
        // disturb the old leader.
        return runner->restart(leader).then(
            [runner] { return runner->checkOneLeader(); });
      })
      .then([runner, servers](auto leader) {
        // if there's no quorum, no leader should
        // be elected.
        auto leader2 = (leader + 1) % servers;
        return seastar::when_all_succeed(runner->kill(leader),
                                         runner->kill(leader2))
            .then([runner] { return runner->checkNoLeader(); })
            .then([leader, leader2] {
              return seastar::make_ready_future<laomd::raft::id_t,
                                                laomd::raft::id_t>(leader,
                                                                   leader2);
            });
      })
      .then([runner](auto last_killed1, auto last_killed2) {
        return seastar::when_all_succeed(runner->restart(last_killed1),
                                         runner->restart(last_killed2))
            .then(
                [runner] { return runner->checkOneLeader().discard_result(); });
      })
      .finally([runner] { return runner->clean_up(); });
}
