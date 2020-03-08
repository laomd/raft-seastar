#pragma once
#include "test_env.hh"
#include <filesystem>
#include <iostream>
#include <raft/interface/raft_types.hh>
#include <seastar/testing/test_case.hh>
#include <seastar/testing/test_runner.hh>
#include <yaml-cpp/yaml.h>

namespace laomd {
namespace raft {

#define RAFT_TEST_CASE(name)                                                   \
  class name : public seastar::testing::seastar_test {                         \
  public:                                                                      \
    const char *get_test_file() override { return __FILE__; }                  \
    const char *get_name() override { return #name; }                          \
    seastar::future<> run_test_case() override;                                \
    seastar::future<> run_case();                                              \
                                                                               \
  private:                                                                     \
    std::unique_ptr<TestEnv> env;                                              \
    int num_servers;                                                           \
    ms_t electionTimeout;                                                      \
  };                                                                           \
  static name name##_instance;                                                 \
  seastar::future<> name::run_test_case() {                                    \
    std::string case_name(#name);                                              \
    if (case_name.find("disable") != std::string::npos) {                      \
      std::cout << "skip test case " #name << std::endl;                       \
      return seastar::make_ready_future();                                     \
    }                                                                          \
    auto cfg_file = __DIR__ / "config_" #name ".yaml";                         \
    if (!std::filesystem::exists(cfg_file)) {                                  \
      cfg_file = __DIR__ / "config.yaml";                                      \
    }                                                                          \
    YAML::Node config = YAML::LoadFile(cfg_file);                              \
    num_servers = config["num_servers"].as<int>(3);                            \
    electionTimeout = ms_t(config["election_timeout"].as<int>(100));           \
    bool clean_data_dir = config["election_timeout"].as<bool>(true);           \
    env = std::make_unique<TestEnv>(                                           \
        get_name(), num_servers, electionTimeout,                              \
        ms_t(config["heartbeat_interval"].as<int>(10)),                        \
        ms_t(config["rpc_timeout"].as<int>(100)),                              \
        config["log_to_stdout"].as<bool>(false));                              \
    return env->start_servers(clean_data_dir)                                  \
        .then([this] { return run_case(); })                                   \
        .finally([this] { return env->clean_up(); });                          \
  }                                                                            \
  seastar::future<> name::run_case()

} // namespace raft
} // namespace laomd