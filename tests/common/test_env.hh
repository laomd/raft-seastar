#pragma once
#include "raft/service/raft_impl.hh"
#include "state_machine/kv_state_machine.hh"
#include <boost/test/auto_unit_test.hpp>
#include <filesystem>
#include <limits>
#include <rpc/utils.hh>
#include <seastar/core/sleep.hh>
#include <sstream>
#include <sys/wait.h>
#include <util/function.hh>
#include <util/net.hh>
#include <vector>
using namespace std::chrono;

namespace laomd {
namespace raft {

class TestEnv {
  DISALLOW_COPY_AND_ASSIGN(TestEnv);

public:
  TestEnv(const std::string &case_name, size_t num_servers, ms_t electionTime,
          ms_t heartbeat, ms_t rpc_timedout, bool log_to_stdout)
      : case_name_(case_name), electionTimeout_(electionTime),
        heartbeat_(heartbeat), rpc_timedout_(rpc_timedout),
        log_to_stdout_(log_to_stdout) {
    fill_stubs(num_servers);
  }

  ~TestEnv() { clean_up(); }

  void clean_up() {
    for (int i = 0; i < server_subpros_.size(); i++) {
      (void)kill(i);
    }
  }

  seastar::future<> kill(id_t id) {
    auto pid = server_subpros_[id];
    if (pid) {
      std::cout << "kill server " << id << ", pid=" << pid << std::endl;
      ::kill(pid, SIGTERM);
      // without waitpid, child progress will be defunct
      waitpid(pid, nullptr, 0);
      server_subpros_[id] = 0;
    }
    return seastar::make_ready_future();
  }

  seastar::future<> restart(id_t id) {
    std::cout << "restart server " << id << std::endl;
    auto s = get_peers_string();
    auto pid = fork_server(s, id, false);
    if (pid != 0) {
      server_subpros_[id] = pid;
      return wait_start();
    } else {
      return seastar::make_ready_future();
    }
  }

  seastar::future<> checkNoLeader(term_t term) {
    std::cout << "checking...There should be no leaders" << std::endl;
    return seastar::parallel_for_each(addrs_, [this, term](auto addr) {
      seastar::rpc::client_options opts;
      opts.send_timeout_data = false;
      auto stub = seastar::make_shared<RaftClient>(opts, addr, rpc_timedout_);
      auto fut = stub->GetState()
                     .then([term](term_t t, id_t id, bool isLeader) {
                       if (isLeader) {
                         if (term == t) {
                           std::string msg =
                               "server " + std::to_string(id) + " is a leader";
                           BOOST_FAIL(msg.c_str());
                         } else {
                           std::string msg = "term changed, server " +
                                             std::to_string(id) +
                                             " is a new leader";
                           BOOST_WARN(msg.c_str());
                         }
                       }
                     })
                     .finally([stub] {
                       return stub->stop().finally(
                           [stub] { return seastar::make_ready_future(); });
                     });
      return ignore_rpc_exceptions(std::move(fut));
    });
  }

  seastar::future<term_t, id_t> checkOneLeader(uint32_t times = 10) {
    auto leaders = seastar::make_shared<std::map<term_t, id_t>>();
    return seastar::do_until(
               [times]() mutable { return !(times--); },
               [this, leaders] {
                 return seastar::sleep(electionTimeout_).then([this, leaders] {
                   return seastar::do_for_each(addrs_, [this, leaders](
                                                           const auto &addr) {
                     seastar::rpc::client_options opts;
                     opts.send_timeout_data = false;
                     auto stub = seastar::make_shared<RaftClient>(
                         opts, addr, rpc_timedout_);
                     auto fut =
                         stub->GetState()
                             .then([leaders](term_t term, id_t id,
                                             bool isLeader) {
                               if (isLeader) {
                                 if (leaders->find(term) == leaders->end()) {
                                   (*leaders)[term] = id;
                                 } else {
                                   BOOST_REQUIRE_EQUAL(leaders->at(term), id);
                                 }
                               }
                             })
                             .finally([stub] {
                               return stub->stop().finally([stub] {
                                 return seastar::make_ready_future();
                               });
                             });
                     return ignore_rpc_exceptions(std::move(fut));
                   });
                 });
               })
        .then([leaders] {
          BOOST_REQUIRE(!leaders->empty());
          auto [term, leader] = *leaders->rbegin();
          std::cout << "term=" << term << ", leader is server " << leader
                    << std::endl;
          return seastar::make_ready_future<term_t, id_t>(term, leader);
        });
  }

  seastar::future<> start_servers(bool clean_data_dir) {
    auto s = get_peers_string();
    size_t num_servers = addrs_.size();
    for (int i = 0; i < num_servers; i++) {
      auto pid = fork_server(s, i, clean_data_dir);
      if (pid == 0) {
        return seastar::make_ready_future();
      } else {
        server_subpros_.emplace_back(pid);
      }
    }
    return wait_start();
  }

  seastar::future<int, seastar::sstring> nCommitted(int index) const {
    return seastar::do_with(
        0, seastar::sstring(),
        [index, this](int &count, seastar::sstring &cmd) {
          return seastar::do_for_each(
                     addrs_,
                     [index, this, &count, &cmd](auto addr) {
                       seastar::rpc::client_options opts;
                       opts.send_timeout_data = false;
                       auto stub = seastar::make_shared<KVStateMachineStub>(
                           opts, addr, rpc_timedout_);
                       auto fut =
                           stub->get(index)
                               .then([&count, &cmd](seastar::sstring cmd1,
                                                    bool ok) {
                                 if (ok) {
                                   BOOST_REQUIRE(count <= 0 || cmd == cmd1);
                                   count++;
                                   cmd = cmd1;
                                 }
                               })
                               .finally([stub] {
                                 return stub->stop().finally([stub] {
                                   return seastar::make_ready_future();
                                 });
                               });
                       return ignore_rpc_exceptions(std::move(fut));
                     })
              .then([&count, &cmd] {
                return seastar::make_ready_future<int, seastar::sstring>(count,
                                                                         cmd);
              });
        });
  }

  seastar::future<int, bool> AppendLog(id_t serverId,
                                       const seastar::sstring &cmd) {
    return RetriableAppendLog(serverId, cmd, rpc_timedout_);
  }

  seastar::future<> Commit(seastar::sstring cmd, int expectedServers) {
    auto fut = seastar::repeat([=] {
      return seastar::do_with(-1, [=](int &index) {
        return seastar::parallel_for_each(boost::irange(addrs_.size()),
                                          [=, &index](int i) {
                                            return AppendLog(i, cmd).then(
                                                [&index](int index1, bool ok) {
                                                  if (ok) {
                                                    index = index1;
                                                  }
                                                });
                                          })
            .then([=, &index] {
              if (index != -1) {
                std::cout << "Append " << cmd << " done, index=" << index
                          << ", wait for it to be committed in at least "
                          << expectedServers << " servers" << std::flush;
                // somebody claimed to be the leader and to have
                // submitted our command; wait a while for agreement.
                auto fut = seastar::repeat([=, &index] {
                  std::cout << ".";
                  return nCommitted(index).then(
                      [expectedServers, cmd](int nd, seastar::sstring cmd1) {
                        if (nd >= expectedServers && cmd == cmd1) {
                          std::cout << std::endl;
                          return seastar::make_ready_future<
                              seastar::stop_iteration>(
                              seastar::stop_iteration::yes);
                        } else {
                          return seastar::sleep(20ms).then(
                              [] { return seastar::stop_iteration::no; });
                        }
                      });
                });
                return with_timeout<std::chrono::steady_clock>(2s,
                                                               std::move(fut))
                    .then_wrapped([](auto fut) {
                      fut.ignore_ready_future();
                      if (fut.failed()) {
                        return seastar::stop_iteration::no;
                      }
                      return seastar::stop_iteration::yes;
                    });
              } else {
                std::cout << "Append " << cmd << " failed, retry" << std::endl;
                return seastar::sleep(50ms).then(
                    [] { return seastar::stop_iteration::no; });
              }
            });
      });
    });
    return with_timeout<std::chrono::steady_clock>(60s, std::move(fut))
        .handle_exception_type([](seastar::timed_out_error &) {
          BOOST_FAIL("timedout error in TestEnv::one.");
        });
  }

private:
  pid_t fork_server(const std::string &all_peers, id_t i,
                    bool clean_data_dir) const {
    pid_t pid = fork();
    if (pid == 0) {
      std::string server_id = std::to_string(i);
      auto data_dir = get_data_dir(i);
      if (clean_data_dir) {
        std::filesystem::remove_all(data_dir);
      }
      std::filesystem::create_directories(data_dir);
      execl("./raft_server", "raft_server", "-c", "10", "--peers",
            all_peers.c_str(), "--data-dir", data_dir.c_str(), "-e",
            std::to_string(electionTimeout_.count()).c_str(), "-b",
            std::to_string(heartbeat_.count()).c_str(), "--me",
            server_id.c_str(), "--log-file",
            (data_dir / ("server.log")).c_str(), NULL);
    }
    return pid;
  }

  seastar::future<> wait_start(int retry_count = 60) {
    std::cout << "waiting all servers to start up" << std::flush;
    return seastar::parallel_for_each(
               addrs_,
               [this, retry_count](const auto &addr) {
                 std::function<seastar::future<term_t, id_t, bool>(uint32_t)>
                     func = [=, duration = 1s](uint32_t count) {
                       auto fut = seastar::make_ready_future();
                       if (count != 0) {
                         fut = seastar::sleep(1s);
                       }
                       return fut.then([this, addr] {
                         std::cout << "waiting " << addr << " to start up..."
                                   << std::endl;
                         seastar::rpc::client_options opts;
                         opts.send_timeout_data = false;
                         auto stub = seastar::make_shared<RaftClient>(
                             opts, addr, rpc_timedout_);
                         return stub->GetState().finally([stub] {
                           return stub->stop().finally(
                               [stub] { return seastar::make_ready_future(); });
                         });
                       });
                     };
                 return with_backoff(retry_count, func).discard_result();
               })
        .then([] { std::cout << "done!!!!" << std::endl; });
  }

  seastar::future<int, bool>
  RetriableAppendLog(id_t serverId, const seastar::sstring &cmd, ms_t timeout) {
    seastar::rpc::client_options opts;
    opts.send_timeout_data = false;
    auto stub =
        seastar::make_shared<RaftClient>(opts, addrs_[serverId], timeout);
    return stub->Append(cmd)
        .then_wrapped([this, serverId, cmd, timeout](auto &&fut) {
          try {
            return seastar::make_ready_future<int, bool>(fut.get());
          } catch (seastar::rpc::timeout_error &) {
            // keep retring on timedout
            return RetriableAppendLog(serverId, cmd, timeout * 2);
          } catch (...) {
            return seastar::make_ready_future<int, bool>(-1, false);
          }
        })
        .finally([stub] {
          return stub->stop().finally(
              [stub] { return seastar::make_ready_future(); });
        });
  }

  std::filesystem::path get_data_dir(id_t i) const {
    return std::filesystem::current_path() / case_name_ /
           ("data" + std::to_string(i));
  }

  std::string get_peers_string() const {
    std::stringstream peers;
    for (const auto &addr : addrs_) {
      peers << addr << ",";
    }

    std::string s = peers.str();
    s.pop_back();
    return s;
  }

  void fill_stubs(size_t num_servers) {
    uint16_t port;
    for (int i = 0; i < num_servers; i++) {
      while ((port = getAvailableListenPort()) == 0)
        ;
      addrs_.emplace_back(seastar::ipv4_addr(port));
    }
  }

  std::vector<seastar::ipv4_addr> addrs_;
  std::vector<pid_t> server_subpros_;
  const std::string case_name_;
  ms_t electionTimeout_, heartbeat_, rpc_timedout_;
  bool log_to_stdout_;
};

} // namespace raft
} // namespace laomd