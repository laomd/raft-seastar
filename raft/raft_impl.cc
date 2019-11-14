#include "raft_impl.hh"
#include "common_generated.h"
#include <chrono>
#include <fstream>
#include <seastar/core/future.hh>
#include <smf/log.h>
#include <smf/rpc_client.h>
#include <system_error>

namespace smf {
class remote_connection_error;
}

BEGIN_NAMESPACE(laomd)
BEGIN_NAMESPACE(raft)

RaftImpl::RaftImpl(uint16_t server_id,
                   const std::vector<seastar::ipv4_addr> &other_servers)
    : Raft(), state_(ServerState_FOLLOWER), server_id_(server_id),
      timer_([this] { return OnTimer(); }), lastHeartbeat_(clock_type::now()),
      currentTerm_(null), votedFor_(null) {
  using namespace std::chrono;
  for (auto &&server : other_servers) {
    smf::rpc_client_opts opts;
    opts.server_addr = server;

    auto client = seastar::make_shared<RaftClient>(std::move(opts));
    seastar::engine().at_exit([client] { return client->stop(); });
    other_servers_.emplace_back(client);
  }
  timer_.arm_periodic(HEART_BEAT_TIMEOUT);
}

seastar::future<smf::rpc_typed_envelope<VoteResponse>>
RaftImpl::RequestVote(smf::rpc_recv_typed_context<VoteRequest> &&rec) {
  using RspType = smf::rpc_typed_envelope<VoteResponse>;
  RspType rsp;
  rsp.data->term = currentTerm_;
  if (rec->term() >= currentTerm_ &&
      (votedFor_ == null || rec->candidateId()) &&
      (rec->lastLogIndex() >= log_.size() &&
       (log_.empty() || rec->lastLogTerm() >= log_.back().first))) {
    votedFor_ = rec->term();
    Persist();
    rsp.data->voteGranted = true;
  }

  rsp.data->voteGranted = false;
  rsp.envelope.set_status(200);
  return seastar::make_ready_future<RspType>(std::move(rsp));
}

void RaftImpl::Persist() const {
  // std::ofstream fout(".meta");
  // fout.write((const char *)currentTerm_, sizeof(currentTerm_));
  // fout.write((const char *)votedFor_, sizeof(votedFor_));
  // fout.close();
}

/*
  To begin an election, a follower increments its current
term and transitions to candidate state. It then votes for
itself and issues RequestVote RPCs in parallel to each of
the other servers in the cluster. A candidate continues in
this state until one of three things happens:
  (a) it wins the election,
  (b) another server establishes itself as leader, or
  (c) a period of time goes by with no winner.
*/
seastar::future<> RaftImpl::StartElection() {
  using namespace std::chrono;
  LOG_INFO("start election from server {}", server_id_);

  currentTerm_++;
  state_ = ServerState_CANDIDATE;
  voted_count_ = 1;
  return seastar::with_timeout(
             clock_type::now() + ELECTION_TIMEOUT,
             seastar::parallel_for_each(
                 other_servers_.begin(), other_servers_.end(),
                 [this](auto stub) {
                   return stub->reconnect().then_wrapped([this,
                                                          stub](auto &&fut) {
                     if (fut.failed()) {
                       return fut.handle_exception([](auto &&) {});
                     } else {
                       smf::rpc_typed_envelope<VoteRequest> req;
                       req.data->term = currentTerm_;
                       req.data->candidateId = server_id_;
                       req.data->lastLogIndex = log_.size();
                       if (log_.empty()) {
                         req.data->lastLogTerm = null;
                       } else {
                         req.data->lastLogTerm = log_.back().first;
                       }
                       return stub->RequestVote(std::move(req.serialize_data()))
                           .then([this, stub](auto &&resp) {
                             if (resp->voteGranted()) {
                               ++voted_count_;
                             }
                             return stub->stop();
                           });
                     }
                   });
                 }))
      .then_wrapped([this](auto &&fut) {
        LOG_INFO("state: {}, get {} voted in {}.", EnumNameServerState(state_),
                 voted_count_, other_servers_.size() + 1);
        if (!fut.failed() && state_ == ServerState_CANDIDATE &&
            voted_count_ > other_servers_.size() / 2) {
          state_ = ServerState_LEADER;
        } else {
          state_ = ServerState_FOLLOWER;
        }
      });
  ;
}

seastar::future<> RaftImpl::OnTimer() {
  using namespace std::chrono;
  switch (state_) {
  case ServerState_LEADER: {
    break;
  }
  case ServerState_CANDIDATE: {
    return StartElection();
  }
  case ServerState_FOLLOWER: {
    if (clock_type::now() >= lastHeartbeat_ + HEART_BEAT_TIMEOUT) {
      return StartElection();
    }
    break;
  }
  default:
    break;
  }
  return seastar::make_ready_future();
}

END_NAMESPACE(raft)
END_NAMESPACE(laomd)