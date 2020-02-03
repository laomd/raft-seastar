#include "test_common.hh"

SEASTAR_TEST_CASE(test) {
  return with_runner([](auto &runner, int num_servers) {
    auto range = boost::irange(3);
    return seastar::do_for_each(
        range.begin(), range.end(), [&runner, num_servers](int i) {
          auto nd = runner.nCommitted(i).first;
          BOOST_REQUIRE_LE(nd, 0);
          return runner.one(seastar::to_sstring(i), num_servers);
        });
  });
}