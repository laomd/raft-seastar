#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(ReElection2A) {
  return with_env([](auto &env, int num_servers, ms_t election_timeout) {
    return env.checkOneLeader()
        .then([&env](laomd::raft::id_t leader) {
          return env.kill(leader).then([&env, leader] {
            // if the leader disconnects, a new one should be elected.
            return env.checkOneLeader().then([leader](auto new_leader) {
              BOOST_REQUIRE_NE(leader, new_leader);
              return leader;
            });
          });
        })
        .then([&env](laomd::raft::id_t leader) {
          // if the old leader rejoins, that shouldn't
          // disturb the old leader.
          return env.restart(leader).then(
              [&env] { return env.checkOneLeader(); });
        })
        .then([&env, num_servers](auto leader) {
          // if there's no quorum, no leader should
          // be elected.
          auto leader2 = (leader + 1) % num_servers;
          return seastar::when_all_succeed(env.kill(leader), env.kill(leader2))
              .then([&env] { return env.checkNoLeader(); })
              .then([leader, leader2] {
                return seastar::make_ready_future<laomd::raft::id_t,
                                                  laomd::raft::id_t>(leader,
                                                                     leader2);
              });
        })
        .then([&env](auto last_killed1, auto last_killed2) {
          return seastar::when_all_succeed(env.restart(last_killed1),
                                           env.restart(last_killed2))
              .then([&env] { return env.checkOneLeader().discard_result(); });
        });
  });
}
