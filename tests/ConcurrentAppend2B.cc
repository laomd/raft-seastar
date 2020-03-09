#include "common/test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(FailNoAgree2B) {
  return env->Commit("10", num_servers).then([this] {
    return env->checkOneLeader().then([this](auto term, auto leader) {
      return seastar::parallel_for_each(boost::irange(1, 4),
                                        [this, leader](int i) {
                                          return env->kill((i + leader) %
                                                           num_servers);
                                        })
          .then([this, leader] {
            return env->AppendLog(leader, "20")
                .then([this](auto index, bool ok) {
                  BOOST_REQUIRE(ok);
                  BOOST_REQUIRE_EQUAL(index, 2);
                  return seastar::sleep(2 * electionTimeout)
                      .then([this, index] {
                        return env->nCommitted(index).then(
                            [this](int n, auto) { BOOST_REQUIRE_EQUAL(n, 0); });
                      });
                });
          })
          .then([this, leader] {
            auto range = boost::irange(1, 4);
            return seastar::parallel_for_each(range, [this, leader](int i) {
              return env->restart((i + leader) % num_servers);
            });
          })
          .then([this] { return env->checkOneLeader(); })
          .then([this](auto term, auto new_leader) {
            return env->AppendLog(new_leader, "30")
                .then([this](auto index, bool ok) {
                  BOOST_REQUIRE(ok);
                  // if new_leader != leader, index = 2
                  // else index = 3
                  BOOST_REQUIRE(index == 2 || index == 3);
                  return seastar::make_ready_future();
                })
                .then([this] { return env->Commit("1000", num_servers); });
          });
    });
  });
}
