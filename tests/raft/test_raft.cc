#include "test_runner.hh"
#include <seastar/core/coroutine.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
#include <yaml-cpp/yaml.h>
using namespace laomd::raft;

seastar::future<>
with_runner(std::function<seastar::future<>(TestRunner &, int)> func) {
  YAML::Node config = YAML::LoadFile(__DIR__ / "config.yaml");
  auto num_servers = config["num_servers"].as<int>();
  auto runner = seastar::make_shared<TestRunner>(
      num_servers, ms_t(config["election_timeout"].as<int>()),
      ms_t(config["heartbeat_interval"].as<int>()),
      ms_t(config["rpc_timeout"].as<int>()),
      config["log_to_stdout"].as<bool>());
  return runner->start_servers()
      .then([runner, num_servers, func = std::move(func)] {
        return func(*runner, num_servers);
      })
      .finally([runner] { return runner->clean_up(); });
}

SEASTAR_TEST_CASE(InitialElection2A) {
  return with_runner([](auto &runner, int) {
    return runner.checkOneLeader().discard_result();
  });
}

SEASTAR_TEST_CASE(ReElection2A) {
  return with_runner([](auto &runner, int num_servers) {
    return runner.checkOneLeader()
        .then([&runner](laomd::raft::id_t leader) {
          return runner.kill(leader).then([&runner, leader] {
            // if the leader disconnects, a new one should be elected.
            return runner.checkOneLeader().then([leader](auto new_leader) {
              BOOST_REQUIRE_NE(leader, new_leader);
              return leader;
            });
          });
        })
        .then([&runner](laomd::raft::id_t leader) {
          // if the old leader rejoins, that shouldn't
          // disturb the old leader.
          return runner.restart(leader).then(
              [&runner] { return runner.checkOneLeader(); });
        })
        .then([&runner, num_servers](auto leader) {
          // if there's no quorum, no leader should
          // be elected.
          auto leader2 = (leader + 1) % num_servers;
          return seastar::when_all_succeed(runner.kill(leader),
                                           runner.kill(leader2))
              .then([&runner] { return runner.checkNoLeader(); })
              .then([leader, leader2] {
                return seastar::make_ready_future<laomd::raft::id_t,
                                                  laomd::raft::id_t>(leader,
                                                                     leader2);
              });
        })
        .then([&runner](auto last_killed1, auto last_killed2) {
          return seastar::when_all_succeed(runner.restart(last_killed1),
                                           runner.restart(last_killed2))
              .then([&runner] {
                return runner.checkOneLeader().discard_result();
              });
        });
  });
}

SEASTAR_TEST_CASE(BasicAgree2B) {
  return with_runner([](auto &runner, int num_servers) {
    auto range = boost::irange(3);
    return seastar::do_for_each(
        range.begin(), range.end(), [&runner, num_servers](int i) {
          return runner.nCommitted(i + 1).then(
              [&runner, num_servers, i](int nd, seastar::sstring cmd1) {
                BOOST_REQUIRE_LE(nd, 0);
                return runner.one("entry" + seastar::to_sstring(i), num_servers);
              });
        });
  });
}