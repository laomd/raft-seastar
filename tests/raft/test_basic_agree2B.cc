#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(BasicAgree2B) {
  return with_env([](auto &env, int num_servers, ms_t election_timeout) {
    auto range = boost::irange(1, 4);
    return seastar::do_for_each(
        range.begin(), range.end(), [&env, num_servers](int i) {
          return env.nCommitted(i).then([&env, num_servers,
                                         i](int nd, seastar::sstring cmd1) {
            BOOST_REQUIRE_LE(nd, 0);
            return env.Commit("entry" + seastar::to_sstring(i), num_servers);
          });
        });
  });
}