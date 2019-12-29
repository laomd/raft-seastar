#pragma once
#include "lao_utils/function.hh"
#include "raft/service.smf.fb.h"
#include "smf/log.h"
#include "smf/rpc_client.h"
#include "smf/rpc_server.h"
#include "smf/rpc_server_args.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <boost/test/tools/old/interface.hpp>
#include <functional>
#include <lao_utils/test.hh>
#include <map>
#include <numeric>
#include <raft/raft_impl.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/distributed.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/sstring.hh>
#include <seastar/net/socket_defs.hh>
#include <shared_mutex>
#include <vector>

namespace laomd {

class config {
  struct end_point {
    seastar::distributed<smf::rpc_server> server;
    raft::RaftClient client;

    explicit end_point(const seastar::ipv4_addr &address)
        : client(smf::rpc_client_opts(make_client_opts(address))) {}

    seastar::future<> init() {
      smf::rpc_server_args args;
      args.rpc_port = client.server_addr.port;
      args.flags |= smf::rpc_server_flags_disable_http_server;
      return server.start(args);
    }

    seastar::future<> stop() {
      return client.stop().finally([this] { return server.stop(); });
    }

  private:
    smf::rpc_client_opts make_client_opts(const seastar::ipv4_addr &addr) {
      smf::rpc_client_opts opts;
      opts.server_addr = addr;
      return opts;
    }
  };

  std::map<raft::id_t, seastar::shared_ptr<end_point>> endPoints_;
  std::vector<seastar::ipv4_addr> addrs_;
  const raft::ms_t electionTimeout_, heartbeartInterval_;
  size_t checkCnt_;

public:
  config(const std::vector<seastar::ipv4_addr> &servers,
         raft::ms_t electionTimeout, raft::ms_t heartbeartInterval,
         size_t checkCnt = 10)
      : addrs_(servers), electionTimeout_(electionTimeout),
        heartbeartInterval_(heartbeartInterval), checkCnt_(checkCnt) {
    for (int i = 0; i < servers.size(); i++) {
      endPoints_[i] = seastar::make_shared<end_point>(servers[i]);
    }
  }

  seastar::future<> init() {
    std::vector<seastar::future<>> futs;
    for (auto &&item : endPoints_) {
      auto id = item.first;
      auto endPoint = item.second;
      co_await endPoint->init();
      co_await endPoint->server.invoke_on_all(
          [this, id](smf::rpc_server &server) {
            return server.template register_service<raft::RaftImpl>(
                id, addrs_, electionTimeout_, heartbeartInterval_);
          });
      co_await endPoint->server.invoke_on_all(&smf::rpc_server::start);
      co_await endPoint->client.connect();
    }
  }

  seastar::future<raft::id_t> checkOneLeader() {
    using namespace std::chrono;
    std::map<raft::term_t, raft::id_t> leaders;
    seastar::shared_mutex mutex;
    for (int i = 0; i < checkCnt_; i++) {
      co_await seastar::sleep(electionTimeout_).then([this, &leaders, &mutex] {
        std::vector<seastar::future<>> futs;
        for (auto &&item : endPoints_) {
          auto &&endPoint = item.second;
          smf::rpc_typed_envelope<raft::GetStateReq> req;
          auto fut =
              endPoint->client.GetState(req.serialize_data())
                  .then([this, &leaders, &mutex](auto &&rsp) {
                    if (rsp->isLeader()) {
                      return seastar::with_lock(
                          mutex, [this, &leaders, term = rsp->term(),
                                  new_server = rsp->serverId()] {
                            LOG_THROW_IF(leaders.find(term) != leaders.end() &&
                                             leaders[term] != new_server,
                                         "term has a server {}!={}", term,
                                         leaders[term], new_server);
                            leaders[term] = new_server;
                          });
                    }
                    return seastar::make_ready_future<>();
                  });
          futs.emplace_back(with_timeout(100ms, std::move(fut)));
        }
        return seastar::when_all(futs.begin(), futs.end()).discard_result();
      });
    }
    BOOST_REQUIRE(!leaders.empty());
    co_return leaders.rbegin()->second;
  }

  seastar::future<> stop(raft::id_t id) { return endPoints_[id]->stop(); }

  seastar::future<> clean_up() {
    fmt::printf("Do Clean Up\n");
    std::vector<seastar::future<>> futs;
    for (auto &&[id, endPoint] : endPoints_) {
      futs.emplace_back(endPoint->stop());
    }
    return seastar::when_all(futs.begin(), futs.end()).discard_result();
  }
}; // namespace laomd

} // namespace laomd
