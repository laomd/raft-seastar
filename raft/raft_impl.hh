#pragma once

#include "common_generated.h"
#include "service.smf.fb.h"
#include "service_generated.h"
#include <chrono>
#include <lao_utils/common.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/core/timer.hh>
#include <smf/log.h>
#include <smf/rpc_filter.h>
#include <smf/rpc_server.h>
#include <vector>

namespace laomd {
namespace raft {
using seastar::future;

using clock_type = std::chrono::steady_clock;
using ms_t = std::chrono::milliseconds;
using term_t = uint64_t;
using id_t = uint64_t;
const id_t VOTENULL = -1;
const term_t TERMNULL = 0;

class RaftImpl : public Raft {
public:
  RaftImpl(id_t serverId, const std::vector<seastar::ipv4_addr> &peers,
           ms_t electionTimeout, ms_t heartbeatInterval);
  virtual seastar::future<smf::rpc_typed_envelope<VoteResponse>>
  RequestVote(smf::rpc_recv_typed_context<VoteRequest> &&rec) override;

  virtual seastar::future<smf::rpc_typed_envelope<AppendEntriesRsp>>
  AppendEntries(smf::rpc_recv_typed_context<AppendEntriesReq> &&rec) override;

  virtual seastar::future<smf::rpc_typed_envelope<GetStateRsp>>
  GetState(smf::rpc_recv_typed_context<GetStateReq> &&rec) override;

  future<> Start();
  future<> Stop();

private:
  // save Raft's persistent state to stable storage,
  // where it can later be retrieved after a crash and restart.
  // see paper's Figure 2 for a description of what should be persistent.
  future<> Persist() const;
  // restore previously persisted state.
  void ReadPersist();

  future<> ConvertToCandidate();
  future<> ConvertToLeader();
  future<> ConvertToFollwer(term_t term);

  future<> LeaderElection();
  void ResetElectionTimer();
  future<> SendHeartBeart() const;

  bool CheckState(ServerState, term_t) const;
  bool CheckLastLog(term_t lastLogTerm, size_t lastLogIndex) const;
  size_t LastLogIndex() const;
  term_t LastLogTerm() const;

private:
  ServerState state_;
  const id_t serverId_;
  std::vector<seastar::shared_ptr<RaftClient>> peers_;
  bool stopped_;
  mutable seastar::shared_mutex lock_;
  seastar::timer<clock_type> electionTimer_;
  const ms_t electionTimeout_;
  const ms_t heartbeatInterval_;

  // Persistent state on all servers: Updated on stable storage before
  // responding to RPCs)
  term_t currentTerm_;
  id_t votedFor_;
  std::vector<LogEntry> log_;
  // Volatile state on all servers:
  size_t commitIndex_;
  size_t lastApplied_;
  // Volatile state on leaders: Reinitialized after election)
  std::vector<size_t> nextIndex_, matchIndex_;
};

} // namespace raft
} // namespace laomd