#pragma once

#include "raft/raft_service.hh"
#include "util/log.hh"
#include <chrono>
#include <seastar/core/future.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/core/shared_ptr.hh>
#include <vector>

namespace laomd {
namespace raft {
using seastar::future;

const id_t VOTENULL = -1;
const term_t TERMNULL = 0;

class RaftImpl : public RaftService {
public:
  RaftImpl(id_t serverId, const std::vector<std::string> &peers,
           ms_t electionTimeout, ms_t heartbeatInterval);
  virtual ~RaftImpl() = default;

  // return currentTerm, serverId and whether granted
  virtual seastar::future<term_t, id_t, bool>
  RequestVote(term_t term, id_t candidateId, term_t llt, size_t lli) override;

  // return currentTerm, serverId and whether is leader
  virtual seastar::future<term_t, id_t, bool> GetState() override;

  virtual void start() override;
  virtual future<> stop() override;

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

  future<> LeaderElection(ms_t);
  void ResetElectionTimer();
  future<> OnElectionTimedout(ServerState state);
  future<> SendHeartBeart() const;

  bool CheckState(ServerState, term_t) const;
  bool CheckLastLog(term_t lastLogTerm, size_t lastLogIndex) const;
  size_t LastLogIndex() const;
  term_t LastLogTerm() const;

private:
  mutable seastar::shared_mutex lock_;
  ServerState state_;
  const id_t serverId_;
  std::vector<seastar::ipv4_addr> peers_;
  bool stopped_;
  seastar::promise<> stopped_pro_;
  seastar::promise<> electionTimer_;
  const ms_t electionTimeout_;
  const ms_t heartbeatInterval_;

  // Persistent state on all servers: Updated on stable storage before
  // responding to RPCs)
  term_t currentTerm_;
  id_t votedFor_;
  std::vector<std::pair<term_t, seastar::sstring>> log_;
  // Volatile state on all servers:
  size_t commitIndex_;
  size_t lastApplied_;
  // Volatile state on leaders: Reinitialized after election)
  std::vector<size_t> nextIndex_, matchIndex_;

  LOG_DECLARE();
};

} // namespace raft
} // namespace laomd