#include <chrono>
#include <lao_utils/test.hh>
#include <seastar/util/defer.hh>
#include <fmt/printf.h>
#include <seastar/testing/test_case.hh>
#include "raft/raft_impl.hh"
#include "config.hh"

namespace laomd {

const auto electionTimeout = raft::ms_t(1000);
const auto heartbeart = raft::ms_t(50);

SEASTAR_TEST_CASE(TestInitialElection2A) {
  size_t servers = 3;
  config cfg(servers, electionTimeout, heartbeart);
  // seastar::defer([&cfg] {
  //   return cfg.clean_up();
  // });
  fmt::print("Test (2A): initial election ...\n");
  
  // is a leader elected?
  co_await cfg.checkOneLeader();
  // does the leader+term stay the same if there is no network failure?
	// auto term1 = cfg.checkTerms()
	// time.Sleep(2 * RaftElectionTimeout)
	// term2 := cfg.checkTerms()
	// if term1 != term2 {
	// 	fmt.Printf("warning: term changed even though there were no failures")
	// }

	fmt::print("  ... Passed\n");
}

}