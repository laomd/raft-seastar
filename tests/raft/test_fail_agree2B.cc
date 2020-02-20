#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(FailAgree2B) {
  return with_env([](auto &env, int num_servers, ms_t election_timeout) {
    return env.Commit("101", num_servers).then([&env, num_servers] {
      return env.checkOneLeader().then([&env, num_servers](auto leader) {
        // follower network disconnection
        auto to_stop = (leader + 1) % num_servers;
        return env.kill(to_stop)
            .then([&env, num_servers] {
              auto range = boost::irange(102, 106);
              return seastar::do_for_each(
                  range.begin(), range.end(), [&env, num_servers](int i) {
                    return env.Commit(seastar::to_sstring(i), num_servers - 1);
                  });
            })
            .then([&env, num_servers, to_stop] {
              return env.restart(to_stop).then([&env, num_servers] {
                return env.Commit("106", num_servers);
              });
            });
      });
    });
  });
}