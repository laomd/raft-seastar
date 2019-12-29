#include "config.hh"
#include "raft/raft_impl.hh"
#include <bits/stdint-uintn.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <functional>
#include <memory>
#include <seastar/core/future.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/thread.hh>
#include <seastar/net/socket_defs.hh>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace laomd {

void operator>>(const YAML::Node &node,
                std::vector<seastar::ipv4_addr> &addrs) {
  BOOST_REQUIRE(node.size() > 1);
  for (auto &&addr : node) {
    addrs.emplace_back(addr["ip"].as<uint16_t>());
  }
}

template <typename Func>
seastar::future<> DoTest(seastar::sstring cfg_path, Func &&func) {
  YAML::Node test_config = YAML::LoadFile(cfg_path);
  std::vector<seastar::ipv4_addr> servers;
  test_config["servers"] >> servers;
  BOOST_REQUIRE_GT(servers.size(), 1);
  auto electionTimeout = raft::ms_t(1000);
  auto heartbeart = raft::ms_t(50);
  size_t checkCnt = 10;
  if (test_config["electionTimeout"]) {
    electionTimeout = raft::ms_t(test_config["electionTimeout"].as<size_t>());
  }
  if (test_config["heartbeartInteval"]) {
    heartbeart = raft::ms_t(test_config["heartbeartInteval"].as<size_t>());
  }
  if (test_config["checkCnt"]) {
    checkCnt = test_config["checkCnt"].as<size_t>();
  }
  config cfg(servers, electionTimeout, heartbeart, checkCnt);
  co_await cfg.init();
  co_await func(cfg);
  co_await cfg.clean_up();
}

SEASTAR_TEST_CASE(TestInitialElection2A) {
  return DoTest("/root/raft-seastar/tests/raft/test_data/"
                "TestInitialElection2A.yml",
                [](auto &cfg) -> seastar::future<> {
                  fmt::print("Test (2A): initial election ...\n");
                  // is a leader elected?
                  return cfg.checkOneLeader().discard_result();
                });
}

SEASTAR_TEST_CASE(TestReElection2A) {
  return DoTest("/root/raft-seastar/tests/raft/test_data/"
                "TestReElection2A.yml",
                [](auto &cfg) -> seastar::future<> {
                  fmt::print("Test (2A): initial election ...\n");
                  // is a leader elected?
                  fmt::print("Test (2A): reelection ...\n");
                  return cfg.checkOneLeader().then([&cfg](auto leader) {
                    // if the leader disconnects, a new one should be elected.
                    fmt::printf("stop leader %d...\n", leader);
                    return cfg.stop(leader).then([&cfg, leader] {
                      fmt::printf("check leader again...\n");
                      return cfg.checkOneLeader().then([&cfg, leader](auto) {
                        // if the old leader rejoins, that shouldn't
                        // disturb the old leader.
                        // cfg->start(leader);
                        // auto leader2 = co_await cfg->checkOneLeader();
                        // BOOST_REQUIRE(leader != leader2);
                      });
                    });
                  });
                });
}

} // namespace laomd
