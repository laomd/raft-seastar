#include "test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(BasicAgree2B) {
  return env->checkOneLeader().then([this](auto leader) {
    auto range = boost::irange(0, 5);
    return seastar::do_for_each(
               range,
               [this, leader](auto i) {
                 return env->AppendLog(leader, seastar::to_sstring(100 + i))
                     .then([](auto index, bool ok) { BOOST_REQUIRE(ok); });
               })
        .then([this] {
          return seastar::do_with(true, [this](bool &success) {
            return seastar::repeat([this, &success] {
              return seastar::parallel_for_each(
                         boost::irange(0, 5),
                         [this, &success](auto i) {
                           return env->nCommitted(i + 1).then(
                               [this, &success, i](auto n, auto cmd) {
                                 if (n != num_servers) {
                                   std::cout << "log entry at index " << i + 1
                                             << " has been committed on " << n
                                             << "(!=" << num_servers
                                             << ") servers" << std::endl;
                                   success = false;
                                 }
                               });
                         })
                  .then([this, &success] {
                    if (success) {
                      return seastar::make_ready_future<
                          seastar::stop_iteration>(
                          seastar::stop_iteration::yes);
                    } else {
                      success = true;
                      return seastar::sleep(2 * electionTimeout).then([] {
                        return seastar::stop_iteration::no;
                      });
                    }
                  });
            });
          });
        });
  });
}