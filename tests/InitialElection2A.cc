#include "test_common.hh"

SEASTAR_TEST_CASE(test) {
  return with_runner(
      [](auto &runner, int) { return runner.checkOneLeader().discard_result(); });
}
