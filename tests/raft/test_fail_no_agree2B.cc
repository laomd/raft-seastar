#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(FailAgree2B) {
  return with_env(
      [](auto &env, int num_servers, ms_t election_timeout) {
        return env.Commit("10", num_servers)
            .then([&env, num_servers, election_timeout] {
              return env.checkOneLeader().then([&env, num_servers,
                                                election_timeout](auto leader) {
                return seastar::parallel_for_each(
                           boost::irange(1, 4),
                           [&env, num_servers, leader](int i) {
                             return env.kill((i + leader) % num_servers);
                           })
                    .then([&env, num_servers, election_timeout, leader] {
                      return env.AppendLog(leader, "20")
                          .then([&env, num_servers,
                                 election_timeout](auto index, bool ok) {
                            BOOST_REQUIRE(ok);
                            BOOST_REQUIRE_EQUAL(index, 2);
                            return seastar::sleep(2 * election_timeout)
                                .then([&env, num_servers, index] {
                                  return env.nCommitted(index).then(
                                      [&env, num_servers](int n, auto) {
                                        BOOST_REQUIRE_EQUAL(n, 0);
                                      });
                                });
                          });
                    })
                    .then([&env, num_servers, leader] {
                      auto range = boost::irange(1, 4);
                      return seastar::do_for_each(
                          range, [&env, num_servers, leader](int i) {
                            return env.restart((i + leader) % num_servers);
                          });
                    })
                    .then([&env, num_servers] { return env.checkOneLeader(); })
                    .then([&env, num_servers](auto new_leader) {
                      return env.AppendLog(new_leader, "30")
                          .then([&env, num_servers](auto index, bool ok) {
                            BOOST_REQUIRE(ok);
                            // if new_leader != leader, index = 2, else index =
                            // 3s
                            BOOST_REQUIRE(index == 2 || index == 3);
                            return seastar::make_ready_future();
                          })
                          .then([&env, num_servers] {
                            return env.Commit("1000", num_servers);
                          });
                    });
              });
            });
      },
      5);
}