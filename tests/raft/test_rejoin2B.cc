#include "test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(FailAgree2B) {
  return env->Commit("101", num_servers).then([this] {
    return env->checkOneLeader().then([this](auto leader) {
      // follower network disconnection
      auto to_stop = (leader + 1) % num_servers;
      return env->kill(to_stop)
          .then([this] {
            auto range = boost::irange(102, 106);
            return seastar::do_for_each(
                range.begin(), range.end(), [this](int i) {
                  return env->Commit(seastar::to_sstring(i), num_servers - 1);
                });
          })
          .then([this, to_stop] {
            return env->restart(to_stop).then(
                [this] { return env->Commit("106", num_servers); });
          });
    });
  });
}