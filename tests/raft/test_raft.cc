#include <chrono>
#include <lao_utils/test.hh>
#include <seastar/util/defer.hh>
#include <fmt/printf.h>
#include <seastar/testing/test_case.hh>
#include "config.hh"

namespace laomd {

const auto electionTimeout = raft::ms_t(1000);
const auto heartbeart = raft::ms_t(50);

SEASTAR_TEST_CASE(TestInitialElection2A) {
  srand(time(nullptr));
  size_t servers = 3;
  config cfg(servers, electionTimeout, heartbeart);
  fmt::print("Test (2A): initial election ...\n");
  
  // is a leader elected?
  co_await cfg.checkOneLeader();
  co_await cfg.clean_up();
}

SEASTAR_TEST_CASE(TestReElection2A) {
  srand(time(nullptr));
  size_t servers = 3;
  config cfg(servers, electionTimeout, heartbeart);
  fmt::print("Test (2A): reelection ...\n");
  auto leader = co_await cfg.checkOneLeader();
  // if the leader disconnects, a new one should be elected.
  fmt::printf("stop leader %d...\n", leader);
  co_await cfg.stop(leader);
  fmt::printf("check leader again...\n");
  co_await cfg.checkOneLeader();
  // if the old leader rejoins, that shouldn't
	// disturb the old leader.
  // cfg.start(leader);
  // auto leader2 = co_await cfg.checkOneLeader();
  // BOOST_REQUIRE(leader != leader2);
  co_await cfg.clean_up();
}

}
