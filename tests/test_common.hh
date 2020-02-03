#pragma once

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
      num_servers,
      ms_t(config["election_timeout"].as<int>()),
      ms_t(config["heartbeat_interval"].as<int>()),
      ms_t(config["rpc_timeout"].as<int>()),
      config["log_to_stdout"].as<bool>());
  return runner->start_servers()
      .then([runner, num_servers, func = std::move(func)] { return func(*runner, num_servers); })
      .finally([runner] { return runner->clean_up(); });
}