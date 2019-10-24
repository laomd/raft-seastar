#include <lao_utils/test.hh>
#include <seastar/core/sleep.hh>

BEGIN_NAMESPACE(laomd)

const int RaftElectionTimeout = 1000;

SEASTAR_TEST_CASE(TestInitialElection2A) {
    using namespace std::chrono_literals;
    co_await seastar::sleep(1s);
}

END_NAMESPACE(laomd)