#include "test_case.hh"
using namespace laomd::raft;

RAFT_TEST_CASE(BasicAgree2B) {
  auto range = boost::irange(1, 4);
  return seastar::do_for_each(range.begin(), range.end(), [this](int i) {
    return env->nCommitted(i).then([this, i](int nd, seastar::sstring cmd1) {
      BOOST_REQUIRE_LE(nd, 0);
      return env->Commit("entry" + seastar::to_sstring(i), num_servers);
    });
  });
}