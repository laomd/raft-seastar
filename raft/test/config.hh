#pragma once
#include "service.smf.fb.h"
#include "smf/rpc_client.h"
#include "smf/rpc_recv_context.h"
#include "smf/rpc_recv_typed_context.h"
#include "smf/rpc_server_args.h"
#include "smf/rpc_typed_envelope.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <boost/mpl/at.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <raft/raft_impl.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sleep.hh>
#include <seastar/net/socket_defs.hh>
#include <string>
#include <utility>
#include <vector>

namespace laomd {

class config {
  std::vector<seastar::shared_ptr<smf::rpc_server>> servers_;
  std::vector<seastar::shared_ptr<raft::RaftClient>> stubs_;

  static std::vector<uint16_t> get_available_ports(int n) {
    std::vector<uint16_t> ports(n);
    std::iota(ports.begin(), ports.end(), 12000);
    return ports;
  }
public:
  config(int n, raft::ms_t electionTimeout, raft::ms_t heartbeartInterval) {
    using namespace std::chrono;
    auto ports = get_available_ports(n);
    for (auto p: ports) {
      smf::rpc_server_args args;
      args.rpc_port = p;
      args.flags |= smf::rpc_server_flags_disable_http_server;
      auto server = seastar::make_shared<smf::rpc_server>(args);
      std::vector<seastar::ipv4_addr> others(n - 1);
      std::copy_if(ports.begin(), ports.end(), others.begin(), std::bind1st(std::not_equal_to<uint16_t>(), p));
      server->register_service<raft::RaftImpl>(p, others, 1000ms, 50ms);
      servers_.emplace_back();

      smf::rpc_client_opts opts;
      opts.server_addr = p;
      stubs_.emplace_back(seastar::make_shared<raft::RaftClient>(opts));
    }
    for (auto&& s: servers_) {
      s->start();
    }
  }

  seastar::future<> checkOneLeader() {
    using namespace std::chrono;
    for (int i = 0; i < 10; i++) {
      co_await seastar::sleep(500ms);
      std::map<raft::term_t, raft::id_t> leaders;
      for (auto&& stub: stubs_) {
        // auto fut = stub->reconnect().then([this, stub, &leaders] {
        //   smf::rpc_typed_envelope<raft::GetStateReq> req;
        //   return stub->GetState(req.serialize_data()).then([this, stub, &leaders] (auto&& rsp) {
        //     if (rsp->isLeader()) {
        //       BOOST_REQUIRE(leaders.find(rsp->term()) == rsp.end());
        //       leaders[rsp->term()] = rsp->serverId();
        //     }
        //   });
        // });
        auto fut = seastar::make_ready_future();
        co_await seastar::with_timeout(raft::clock_type::now() + 100ms, std::move(fut)).handle_exception([](auto&& exp) {
          try {
          std::rethrow_exception(exp);
          } catch (seastar::timed_out_error& e) {
          
          }
        });
      }
    }
  }
};

}