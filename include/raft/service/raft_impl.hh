#pragma once

#include "raft/interface/ilog_applier.hh"
#include "raft/service/raft_service.hh"
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
           ms_t electionTimeout, ms_t heartbeatInterval,
           const std::string &data_dir, ILogApplier *log_applier);
  virtual ~RaftImpl() = default;

  // return currentTerm, serverId and whether granted
  virtual seastar::future<term_t, id_t, bool>
  RequestVote(term_t term, id_t candidateId, term_t llt, int lli) override;

  // return currentTerm, Success
  virtual seastar::future<term_t, bool>
  AppendEntries(term_t term, id_t leaderId, term_t plt, int pli,
                const std::vector<LogEntry> &entries,
                int leaderCommit) override;

  // return currentTerm, serverId and whether is leader
  virtual seastar::future<term_t, id_t, bool> GetState() override;

  // return index, ok
  virtual seastar::future<int, bool> Append(const seastar::sstring &) override;

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
  future<> ConvertToFollower(term_t term);

  future<> LeaderElection(ms_t);
  future<> ApplyLogs();
  future<> AdvanceCommitIndex();
  future<> StartAppendEntries(ms_t);
  void ResetElectionTimer();
  future<> OnElectionTimedout(ServerState state);
  future<> SendHeartBeart() const;

  bool CheckState(ServerState, term_t) const;
  bool CheckLastLog(term_t lastLogTerm, int lastLogIndex) const;
  int LastLogIndex() const;
  term_t LastLogTerm() const;
  int PrevLogIndex(int) const;
  term_t PrevLogTerm(int) const;

private:
  mutable seastar::shared_mutex lock_;
  const std::string meta_file_;
  const id_t serverId_;
  const ms_t electionTimeout_;
  const ms_t heartbeatInterval_;
  ILogApplier *log_applier_;
  std::vector<seastar::ipv4_addr> peers_;
  bool stopped_;
  seastar::promise<> stopped_pro_;
  seastar::promise<> electionTimer_;

  // Persistent state on all servers: Updated on stable storage before
  // responding to RPCs)
  ServerState state_;
  term_t currentTerm_;
  id_t votedFor_;
  std::vector<LogEntry> log_;
  // Volatile state on all servers:
  int commitIndex_;
  int lastApplied_;
  id_t leaderId_;
  // Volatile state on leaders: Reinitialized after election)
  std::vector<int> nextIndex_, matchIndex_;

  LOG_DECLARE();
};

} // namespace raft
} // namespace laomd