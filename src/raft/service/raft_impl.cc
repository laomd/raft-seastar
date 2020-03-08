#include "raft/service/raft_impl.hh"
#include "util/function.hh"
#include "util/log.hh"
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <seastar/core/fstream.hh>
#include <seastar/core/sleep.hh>
#include <utility>

namespace laomd {
namespace raft {

LOG_SETUP(RaftImpl);

RaftImpl::RaftImpl(id_t serverId, const std::vector<std::string> &peers,
                   ms_t electionTimeout, ms_t heartbeatInterval,
                   const std::string &data_dir, ILogApplier *log_applier)
    : meta_file_(data_dir + "/META"), serverId_(serverId),
      electionTimeout_(electionTimeout), heartbeatInterval_(heartbeatInterval),
      log_applier_(log_applier) {
  for (int i = 0; i < peers.size(); i++) {
    if (i != serverId) {
      peers_.emplace_back(peers[i]);
    }
  }
  std::filesystem::create_directories(data_dir);
}

future<> RaftImpl::OnElectionTimedout(ServerState state) {
  return seastar::with_lock(lock_, [=] {
    ResetElectionTimer();
    LOG_INFO("server={}, state={}, election timedout", serverId_,
             EnumNameServerState(state));
    return ConvertToCandidate();
  });
}

void RaftImpl::start() {
  stopped_ = false;
  matchIndex_.resize(peers_.size() + 1, 0);
  nextIndex_.resize(peers_.size() + 1, 0);
  ReadPersist();

  // run backgroud, caller should call stop() to sync
  (void)seastar::do_until(
      [this] { return stopped_; },
      [this] {
        return seastar::with_lock(lock_, [this] { return state_; })
            .then([this](ServerState state) {
              auto electionTimeout =
                  electionTimeout_ + ms_t(rand() % electionTimeout_.count());
              switch (state) {
              case ServerState::CANDIDATE:
                (void)LeaderElection(electionTimeout);
              case ServerState::FOLLOWER:
                return with_timeout<seastar::rpc::rpc_clock_type>(
                           electionTimeout, electionTimer_.get_future())
                    .handle_exception_type(
                        [this, state](seastar::timed_out_error &) {
                          return OnElectionTimedout(state);
                        });
              case ServerState::LEADER:
                return StartAppendEntries(electionTimeout).then([this] {
                  return seastar::sleep(heartbeatInterval_);
                });
              default:
                LOG_ERROR("server={}, invalid state {}", serverId_,
                          EnumNameServerState(state));
                return seastar::make_ready_future();
              }
            });
      })
      .then_wrapped([this](auto &&fut) {
        if (fut.failed()) {
          stopped_pro_.set_exception(fut.get_exception());
        } else {
          stopped_pro_.set_value();
        }
      });
}

future<> RaftImpl::LeaderElection(ms_t time_out) {
  return with_lock(
             lock_,
             [this] {
               return seastar::make_ready_future<ServerState, term_t, int, int>(
                   state_, currentTerm_, LastLogTerm(), LastLogIndex());
             })
      .then([=](ServerState state, term_t term, term_t llt, int lli) {
        if (state_ != ServerState::CANDIDATE) {
          return seastar::make_ready_future();
        }

        auto numVoted = std::make_shared<std::atomic<int>>(1);
        return seastar::parallel_for_each(
            peers_.begin(), peers_.end(), [=](auto addr) mutable {
              auto peer = make_client(addr, time_out);
              auto fut =
                  peer->RequestVote(term, serverId_, llt, lli)
                      .then([this, numVoted, term](term_t rsp_term, id_t addr,
                                                   bool vote) {
                        return seastar::with_lock(lock_, [=] {
                          if (rsp_term > currentTerm_) {
                            LOG_INFO("server={}, receive larger term "
                                     "{}>{}",
                                     serverId_, rsp_term, term);
                            return ConvertToFollower(rsp_term);
                          }
                          if (!CheckState(ServerState::CANDIDATE, term)) {
                            LOG_INFO("server={}, state={}, check "
                                     "state failed",
                                     serverId_, EnumNameServerState(state_));
                            return seastar::make_ready_future();
                          }
                          if (vote) {
                            (*numVoted)++;
                            LOG_INFO("server={}, vote from {}", serverId_,
                                     addr);
                          }
                          if (*numVoted > (peers_.size() + 1) / 2) {
                            LOG_INFO("server({}) win vote", serverId_);
                            return ConvertToLeader().then(
                                [this] { return ResetElectionTimer(); });
                          }
                          return seastar::make_ready_future();
                        });
                      })
                      .finally([peer] {
                        // client shouldn't be destroyed before client::stop is
                        // done
                        return peer->stop().finally(
                            [peer] { return seastar::make_ready_future(); });
                      });
              return ignore_rpc_exceptions(std::move(fut));
            });
      });
}

future<> RaftImpl::StartAppendEntries(ms_t rpc_timeout) {
  return seastar::parallel_for_each(boost::irange(peers_.size()), [=](int i) {
    (void)seastar::repeat([=] {
      return lock_.lock().then([=] {
        if (state_ != ServerState::LEADER) {
          lock_.unlock();
          return seastar::make_ready_future().then(
              [] { return seastar::stop_iteration::yes; });
        }
        auto term = currentTerm_;
        auto plt = PrevLogTerm(i);
        auto pli = PrevLogIndex(i);
        auto commitIndex = commitIndex_;
        auto nextIndex = nextIndex_[i];
        std::vector<LogEntry> entries;
        std::copy(log_.begin() + nextIndex, log_.end(),
                  std::back_inserter(entries));
        lock_.unlock();
        auto size = entries.size();
        auto peer = make_client(peers_[i], rpc_timeout);
        auto stop = seastar::make_lw_shared<bool>(true);
        auto fut =
            peer->AppendEntries(term, serverId_, plt, pli, entries, commitIndex)
                .then([peer, term, i, pli, size, stop, this](term_t rsp_term,
                                                             bool success) {
                  return seastar::with_lock(lock_, [=] {
                    if (rsp_term > currentTerm_) {
                      return ConvertToFollower(rsp_term);
                    } else {
                      if (!CheckState(ServerState::LEADER, term)) {
                        return seastar::make_ready_future();
                      }
                      if (success) {
                        matchIndex_[i] = pli + size;
                        nextIndex_[i] = matchIndex_[i] + 1;
                        DLOG_INFO("server={}, appendEntries success, "
                                  "nextIndex:{}, matchIndex:{}",
                                  serverId_, nextIndex_[i], matchIndex_[i]);
                        return AdvanceCommitIndex();
                      } else {
                        nextIndex_[i]--;
                        *stop = false;
                        return seastar::make_ready_future();
                      }
                    }
                  });
                })
                .finally([peer] {
                  return peer->stop().finally(
                      [peer] { return seastar::make_ready_future(); });
                });
        return ignore_rpc_exceptions(std::move(fut)).then([stop] {
          if (*stop) {
            return seastar::stop_iteration::yes;
          } else {
            return seastar::stop_iteration::no;
          }
        });
      });
    });
  });
}

/*
Receiver implementation:
1. Reply false if term < currentTerm (§5.1)
2. If votedFor is null or candidateId, and candidate’s log is at
least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
*/
seastar::future<term_t, id_t, bool>
RaftImpl::RequestVote(term_t term, id_t candidateId, term_t llt, int lli) {
  return seastar::with_lock(lock_, [=] {
    auto fut = [=] {
      bool voteGranted = false;
      if (term == currentTerm_ &&
          (votedFor_ == VOTENULL || votedFor_ == candidateId) &&
          CheckLastLog(llt, lli)) {
        voteGranted = true;
        votedFor_ = candidateId;
        state_ = ServerState::FOLLOWER;
        ResetElectionTimer();
        LOG_INFO("server={}, vote {}, term:{}, candidate term:{}", serverId_,
                 votedFor_, currentTerm_, term);
      }
      return seastar::make_ready_future<term_t, id_t, bool>(
          currentTerm_, serverId_, voteGranted);
    };
    if (term > currentTerm_) {
      return ConvertToFollower(term).then(fut);
    } else {
      return fut();
    }
  });
}

seastar::future<term_t, bool>
RaftImpl::AppendEntries(term_t term, id_t leaderId, term_t plt, int pli,
                        const std::vector<LogEntry> &entries,
                        int leaderCommit) {
  return seastar::with_lock(lock_, [=] {
    auto func = [=] {
      bool success = false;
      if (term == currentTerm_) {
        auto lli = LastLogIndex();
        if (pli < 0 || (pli <= lli && plt == log_[pli].term)) {
          success = true;
          leaderId_ = leaderId;
          for (int i = 0; i < entries.size(); i++) {
            const auto &entry = entries[i];
            if (entry.index > lli || entry.term != log_[entry.index].term) {
              log_.resize(entry.index);
              for (int j = i; j < entries.size(); j++) {
                log_.emplace_back(entries[j]);
                LOG_INFO("append entry {}", entries[j]);
              }
              break;
            }
          }
        }
      }
      auto func = [success, this] {
        return seastar::make_ready_future<term_t, bool>(currentTerm_, success);
      };
      if (leaderCommit > commitIndex_) {
        commitIndex_ = std::min(leaderCommit, LastLogIndex());
        return ApplyLogs().then(func);
      } else {
        return func();
      }
    };
    if (term >= currentTerm_) {
      return ConvertToFollower(term)
          .then([this] { return ResetElectionTimer(); })
          .then(func);
    } else {
      return func();
    }
  });
}

future<> RaftImpl::ApplyLogs() {
  return seastar::do_until(
      [this] { return lastApplied_ >= commitIndex_; },
      [this] {
        lastApplied_++;
        LOG_INFO(
            "server({}) applyLogs, commitIndex:{}, lastApplied:{}, command:{}",
            serverId_, commitIndex_, lastApplied_, log_[lastApplied_].log);
        auto entry = log_[lastApplied_];
        return log_applier_->apply(entry);
      });
}

future<> RaftImpl::AdvanceCommitIndex() {
  auto matchIndexes = matchIndex_;
  matchIndexes[serverId_] = log_.size() - 1;
  std::sort(matchIndexes.begin(), matchIndexes.end());

  auto N = matchIndexes[(peers_.size() + 1) / 2];
  if (state_ == ServerState::LEADER && N > commitIndex_ &&
      log_[N].term == currentTerm_) {
    LOG_INFO("Server({}) advanceCommitIndex ({} => {})", serverId_,
             commitIndex_, N);
    commitIndex_ = N;
    return ApplyLogs();
  }
  return seastar::make_ready_future();
}

seastar::future<> RaftImpl::Persist() const {
  std::ofstream fout(meta_file_ + ".tmp");
  fout << currentTerm_ << ' ' << (uint32_t)state_ << ' ' << votedFor_ << std::endl;
  for (auto &entry : log_) {
    fout << entry << std::endl;
  }
  fout.close();
  std::filesystem::remove(meta_file_);
  std::filesystem::rename(meta_file_ + ".tmp", meta_file_);
  return seastar::make_ready_future();
}

void RaftImpl::ResetElectionTimer() {
  electionTimer_.set_value();
  electionTimer_ = seastar::promise<>();
}

void RaftImpl::ReadPersist() {
  currentTerm_ = TERMNULL;
  auto state = (uint32_t)ServerState::FOLLOWER;
  votedFor_ = VOTENULL;
  std::ifstream fin(meta_file_);
  fin >> currentTerm_ >> state >> votedFor_;
  state_ = (ServerState)state;

  LogEntry entry;
  while (fin >> entry) {
    log_.emplace_back(entry);
  }
  if (log_.empty()) {
    log_.emplace_back(LogEntry{TERMNULL, 0, "placeholder"});
  }
}

future<> RaftImpl::ConvertToCandidate() {
  currentTerm_++;
  LOG_INFO("Convert server({}) state({}=>{}) term({})", serverId_,
           EnumNameServerState(state_),
           EnumNameServerState(ServerState::CANDIDATE), currentTerm_);
  state_ = ServerState::CANDIDATE;
  votedFor_ = serverId_;
  return Persist();
}

future<> RaftImpl::ConvertToLeader() {
  if (state_ == ServerState::CANDIDATE) {
    LOG_INFO("Convert server({}) state({}=>{}) term {}", serverId_,
             EnumNameServerState(state_),
             EnumNameServerState(ServerState::LEADER), currentTerm_);
    state_ = ServerState::LEADER;
    std::fill(nextIndex_.begin(), nextIndex_.end(), LastLogIndex() + 1);
    std::fill(matchIndex_.begin(), matchIndex_.end(), 0);
    return Persist();
  }
  return seastar::make_ready_future();
}

future<> RaftImpl::ConvertToFollower(term_t term) {
  DLOG_INFO("Convert server({}) state({}=>{}) term({} => {})", serverId_,
            EnumNameServerState(state_),
            EnumNameServerState(ServerState::FOLLOWER), currentTerm_, term);
  state_ = ServerState::FOLLOWER;
  currentTerm_ = term;
  votedFor_ = VOTENULL;
  return Persist();
}

bool RaftImpl::CheckState(ServerState state, term_t term) const {
  return state_ == state && currentTerm_ == term;
}

bool RaftImpl::CheckLastLog(term_t lastLogTerm, int lastLogIndex) const {
  term_t myLastLogTerm = LastLogTerm();
  return lastLogTerm > myLastLogTerm ||
         (lastLogTerm == myLastLogTerm && lastLogIndex >= LastLogIndex());
}

int RaftImpl::LastLogIndex() const { return log_.size() - 1; }

term_t RaftImpl::LastLogTerm() const {
  int lli = LastLogIndex();
  if (lli == 0) {
    return TERMNULL;
  } else {
    return log_[lli].term;
  }
}

int RaftImpl::PrevLogIndex(int idx) const { return nextIndex_[idx] - 1; }

term_t RaftImpl::PrevLogTerm(int idx) const {
  auto prevLogIndex = PrevLogIndex(idx);
  if (prevLogIndex < 0) {
    return TERMNULL;
  } else {
    return log_[prevLogIndex].term;
  }
}

future<> RaftImpl::stop() {
  LOG_INFO("stop server {}", serverId_);
  ResetElectionTimer();
  stopped_ = true;
  return stopped_pro_.get_future();
}

seastar::future<term_t, id_t, bool> RaftImpl::GetState() {
  return seastar::with_lock(lock_, [this] {
    return seastar::make_ready_future<term_t, id_t, bool>(
        currentTerm_, serverId_, state_ == ServerState::LEADER);
  });
}

// return index, ok
seastar::future<int, bool> RaftImpl::Append(const seastar::sstring &cmd) {
  return seastar::with_lock(lock_, [=] {
    if (state_ == ServerState::LEADER) {
      auto index = LastLogIndex() + 1;
      LOG_INFO("server={}, appending entry {}", serverId_, cmd);
      log_.emplace_back(LogEntry{currentTerm_, index, cmd});
      return Persist().then([index, this] {
        return seastar::make_ready_future<int, bool>(index, true);
      });
    } else {
      return seastar::make_ready_future<int, bool>(-1, false);
    }
  });
}

} // namespace raft
} // namespace laomd
