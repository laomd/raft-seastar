#pragma once
#include "raft/service/logentry_applier.hh"
#include "raft/service/raft_impl.hh"
#include <boost/test/auto_unit_test.hpp>
#include <deque>
#include <filesystem>
#include <seastar/core/sleep.hh>
#include <sstream>
#include <sys/wait.h>
#include <util/function.hh>
#include <util/macros.hh>
#include <util/net.hh>
using namespace std::chrono;

namespace laomd {
namespace raft {

class TestRunner {
  DISALLOW_COPY_AND_ASSIGN(TestRunner);

public:
  TestRunner(size_t num_servers, ms_t electionTime, ms_t heartbeat,
             ms_t rpc_timedout, bool log_to_stdout)
      : electionTimeout_(electionTime), heartbeat_(heartbeat),
        rpc_timedout_(rpc_timedout), log_to_stdout_(log_to_stdout) {
    fill_stubs(num_servers);
  }

  ~TestRunner() { clean_up(); }

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
    auto pid = fork_server(s, id);
    if (pid != 0) {
      server_subpros_[id] = pid;
      return wait_start();
    } else {
      return seastar::make_ready_future();
    }
  }

  seastar::future<> checkNoLeader() {
    std::cout << "checking...There should be no leaders" << std::endl;
    return seastar::make_ready_future();
    return seastar::parallel_for_each(addrs_, [this](auto addr) {
      seastar::rpc::client_options opts;
      opts.send_timeout_data = false;
      auto stub = seastar::make_shared<RaftClient>(opts, addr, rpc_timedout_);
      auto fut = stub->GetState()
                     .then([](term_t term, id_t id, bool isLeader) {
                       BOOST_REQUIRE(!isLeader);
                     })
                     .finally([stub] {
                       return stub->stop().finally(
                           [stub] { return seastar::make_ready_future(); });
                     });
      return ignore_rpc_exceptions(std::move(fut));
    });
  }

  seastar::future<id_t> checkOneLeader(uint32_t times = 10) {
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
          auto leader = leaders->rbegin()->second;
          std::cout << "leader is server " << leader << std::endl;
          return leader;
        });
  }

  seastar::future<> start_servers() {
    auto s = get_peers_string();
    size_t num_servers = addrs_.size();
    for (int i = 0; i < num_servers; i++) {
      auto pid = fork_server(s, i);
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
                       auto stub = seastar::make_shared<LogEntryApplierStub>(
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

  seastar::future<> one(seastar::sstring cmd, int expectedServers) {
    auto fut = seastar::repeat([=] {
      return seastar::do_with(-1, [=](int &index) {
        return seastar::parallel_for_each(
                   addrs_,
                   [=, &index](auto addr) {
                     seastar::rpc::client_options opts;
                     opts.send_timeout_data = false;
                     auto stub = seastar::make_shared<RaftClient>(
                         opts, addr, rpc_timedout_);
                     auto fut = stub->Append(cmd)
                                    .then([&index](auto index1, bool ok) {
                                      if (ok) {
                                        index = index1;
                                      }
                                    })
                                    .finally([stub] {
                                      return stub->stop().finally([stub] {
                                        return seastar::make_ready_future();
                                      });
                                    });
                     return ignore_rpc_exceptions(std::move(fut));
                   })
            .then([=, &index] {
              if (index != -1) {
                std::cout << "Append " << cmd << " done, index=" << index
                          << ", wait for it to be committed in at least "
                          << expectedServers << " servers";
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
    return with_timeout<std::chrono::steady_clock>(10s, std::move(fut))
        .handle_exception_type([](seastar::timed_out_error &) {
          BOOST_FAIL("timedout error in TestRunner::one.");
        });
  }

private:
  pid_t fork_server(const std::string &s, id_t i) const {
    pid_t pid = fork();
    if (pid == 0) {
      std::string tmp = std::to_string(i);
      std::string log_file = log_to_stdout_ ? "stdout" : (tmp + ".log");
      execl("../../src/raft/raft_server", "raft_server", "-c", "10", "-p",
            s.c_str(), "-e", std::to_string(electionTimeout_.count()).c_str(),
            "-b", std::to_string(heartbeat_.count()).c_str(), "-i", tmp.c_str(),
            "-l", log_file.c_str(), NULL);
    }
    return pid;
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

  seastar::future<> wait_start() {
    std::cout << "waiting all servers to start up";
    return seastar::repeat([this] {
      return seastar::sleep(electionTimeout_).then([this] {
        auto success = seastar::make_lw_shared<bool>(true);
        return seastar::do_for_each(
                   addrs_,
                   [this, success](auto addr) {
                     if (!(*success)) {
                       return seastar::make_ready_future();
                     }
                     seastar::rpc::client_options opts;
                     opts.send_timeout_data = false;
                     auto stub = seastar::make_shared<RaftClient>(
                         opts, addr, rpc_timedout_);
                     auto fut = stub->GetState()
                                    .then_wrapped([success, addr](auto fut) {
                                      if (fut.failed()) {
                                        *success = false;
                                        std::cout << ".";
                                        return seastar::make_exception_future(
                                            fut.get_exception());
                                      }
                                      return seastar::make_ready_future();
                                    })
                                    .finally([stub] {
                                      return stub->stop().finally([stub] {
                                        return seastar::make_ready_future();
                                      });
                                    });
                     return ignore_rpc_exceptions(std::move(fut));
                   })
            .then_wrapped([this, success](auto fut) {
              if (*success) {
                std::cout << std::endl;
                return seastar::stop_iteration::yes;
              } else {
                return seastar::stop_iteration::no;
              }
            });
      });
    });
  }

  void fill_stubs(size_t num_servers) {
    uint16_t port;
    for (int i = 0; i < num_servers; i++) {
      while ((port = getAvailableListenPort()) == 0)
        ;
      addrs_.emplace_back(seastar::ipv4_addr(port));
    }
  }

  std::deque<seastar::ipv4_addr> addrs_;
  std::deque<pid_t> server_subpros_;
  ms_t electionTimeout_, heartbeat_, rpc_timedout_;
  bool log_to_stdout_;
};

} // namespace raft
} // namespace laomd