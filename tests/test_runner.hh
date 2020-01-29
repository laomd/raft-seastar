#pragma once
#include <boost/test/auto_unit_test.hpp>
#include <deque>
#include <filesystem>
#include <raft/raft_impl.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/sleep.hh>
#include <sstream>
#include <util/function.hh>
using namespace std::filesystem;

namespace laomd {
namespace raft {

class TestRunner {
public:
  TestRunner(size_t num_servers, ms_t electionTime, ms_t heartbeat)
      : electionTimeout_(electionTime), heartbeat_(heartbeat) {
    fill_stubs(num_servers);
    start_servers();
    proto_.register_handler(3, [] {
      return seastar::make_ready_future<term_t, id_t, bool>(0, 0, false);
    });
  }

  ~TestRunner() { clean_up(); }

  void clean_up() { system("kill -9 `pidof raft_server`"); }

  seastar::future<> checkOneLeader() {
    return seastar::repeat([this] {
      return seastar::sleep(electionTimeout_).then([this] {
        std::cout << "checking leaders..." << std::endl;
        auto leaders = seastar::make_shared<std::map<term_t, id_t>>();
        return seastar::do_for_each(
                   addrs_,
                   [this, leaders](const auto &addr) {
                     seastar::rpc::client_options opts;
                     opts.send_timeout_data = false;
                     auto stub = seastar::make_shared<RaftClient>(proto_, opts, addr);
                     auto func = proto_.make_client<
                         seastar::future<term_t, id_t, bool>()>(3);
                     return func(*stub)
                         .then([leaders](term_t term, id_t id, bool isLeader) {
                           if (isLeader) {
                            //  BOOST_REQUIRE(leaders->find(term) ==
                            //                    leaders->end() ||
                            //                (*leaders)[term] == id);
                             (*leaders)[term] = id;
                           }
                         })
                         .finally([stub] {
                           return stub->stop().finally(
                               [stub] { return seastar::make_ready_future(); });
                         })
                         .handle_exception_type(
                             ignore_exception<seastar::rpc::closed_error>)
                         .handle_exception_type(
                             ignore_exception<std::system_error>);
                   })
            .then_wrapped([this, leaders](auto &&fut) {
              std::cout << "leaders: ";
              if (leaders->empty()) {
                std::cout << "none" << std::endl;
                return seastar::stop_iteration::no;
              } else {
                for (auto &&item : *leaders) {
                  std::cout << item.first << "->" << item.second << ' ';
                }
                std::cout << std::endl;
                return seastar::stop_iteration::yes;
              }
            });
      });
    });
  }

private:
  void fill_stubs(size_t num_servers) {
    for (int i = 0; i < num_servers; i++) {
      addrs_.emplace_back(seastar::make_ipv4_address(0, 12000 + i));
    }
  }

  void start_servers() {
    clean_up();
    std::stringstream peers;
    size_t num_servers = addrs_.size();
    for (const auto &addr : addrs_) {
      peers << addr << ",";
    }

    std::string s = peers.str();
    s.pop_back();
    std::string bin = "nohup ../raft/raft_server -c 10 -p " + s + " -e " +
                      std::to_string(electionTimeout_.count()) + " -b " +
                      std::to_string(heartbeat_.count());
    for (int i = 0; i < num_servers; i++) {
      std::string tmp = std::to_string(i);
      std::string cmd = bin + " -i " + tmp + " > " + tmp + ".log &";
      std::cout << "run cmd: " << cmd << std::endl;
      system(cmd.c_str());
    }
  }

  std::deque<seastar::ipv4_addr> addrs_;
  rpc_protocol proto_;
  ms_t electionTimeout_, heartbeat_;
};

} // namespace raft
} // namespace laomd