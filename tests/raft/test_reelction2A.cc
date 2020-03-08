#include "test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(ReElection2A) {
  return env->checkOneLeader()
      .then([this](laomd::raft::id_t leader) {
        return env->kill(leader).then([this, leader] {
          // if the leader disconnects, a new one should be elected.
          return env->checkOneLeader().then([leader](auto new_leader) {
            BOOST_REQUIRE_NE(leader, new_leader);
            return leader;
          });
        });
      })
      .then([this](laomd::raft::id_t leader) {
        // if the old leader rejoins, that shouldn't
        // disturb the old leader.
        return env->restart(leader).then(
            [this] { return env->checkOneLeader(); });
      })
      .then([this](auto leader) {
        // if there's no quorum, no leader should
        // be elected.
        auto leader2 = (leader + 1) % num_servers;
        return seastar::when_all_succeed(env->kill(leader), env->kill(leader2))
            .then([this] { return env->checkNoLeader(); })
            .then([leader, leader2] {
              return seastar::make_ready_future<laomd::raft::id_t,
                                                laomd::raft::id_t>(leader,
                                                                   leader2);
            });
      })
      .then([this](auto last_killed1, auto last_killed2) {
        return seastar::when_all_succeed(env->restart(last_killed1),
                                         env->restart(last_killed2))
            .then([this] { return env->checkOneLeader().discard_result(); });
      });
}
