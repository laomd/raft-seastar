#include "test_env.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
using namespace laomd::raft;

SEASTAR_TEST_CASE(BasicAgree2B) {
  return with_env([](auto &env, int num_servers, ms_t election_timeout) {
    return env.checkOneLeader().then([&env, num_servers,
                                      election_timeout](auto leader) {
      auto range = boost::irange(0, 5);
      return seastar::do_for_each(
                 range,
                 [&env, num_servers, leader](auto i) {
                   return env.AppendLog(leader, seastar::to_sstring(100 + i))
                       .then([](auto index, bool ok) { BOOST_REQUIRE(ok); });
                 })
          .then([&env, num_servers, election_timeout] {
            return seastar::do_with(true, [&env, num_servers,
                                           election_timeout](bool &success) {
              return seastar::repeat([&env, num_servers, election_timeout,
                                      &success] {
                return seastar::parallel_for_each(
                           boost::irange(0, 5),
                           [&env, num_servers, &success](auto i) {
                             return env.nCommitted(i + 1).then(
                                 [num_servers, &success, i](auto n, auto cmd) {
                                   if (n != num_servers) {
                                     std::cout << "log entry at index " << i + 1
                                               << " has been committed on " << n
                                               << "(!=" << num_servers
                                               << ") servers" << std::endl;
                                     success = false;
                                   }
                                 });
                           })
                    .then([&env, num_servers, &success, election_timeout] {
                      if (success) {
                        return seastar::make_ready_future<
                            seastar::stop_iteration>(
                            seastar::stop_iteration::yes);
                      } else {
                        success = true;
                        return seastar::sleep(2 * election_timeout).then([] {
                          return seastar::stop_iteration::no;
                        });
                      }
                    });
              });
            });
          });
    });
  });
}