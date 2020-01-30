#include "raft_impl.hh"
#include "util/function.hh"
#include "util/log.hh"
#include <cstdlib>
#include <ctime>
#include <memory>
#include <seastar/core/sleep.hh>
#include <utility>

namespace laomd {
namespace raft {

LOG_SETUP(RaftService);

RaftService::RaftService(id_t serverId, const std::vector<std::string> &peers,
                         ms_t electionTimeout, ms_t heartbeatInterval)
    : rpc_(), server_(rpc_, seastar::ipv4_addr(peers[serverId])),
      electionTimeout_(electionTimeout), heartbeatInterval_(heartbeatInterval),
      serverId_(serverId) {
  for (int i = 0; i < peers.size(); i++) {
    if (i != serverId) {
      peers_.emplace_back(peers[i]);
    }
  }
  rpc_.register_handler(
      1, [this](term_t term, id_t candidateId, term_t llt, size_t lli) {
        return RequestVote(term, candidateId, llt, lli);
      });
  rpc_.register_handler(3, [this] { return GetState(); });
}

seastar::shared_ptr<RaftClient>
RaftService::make_client(const seastar::ipv4_addr &remote_addr, ms_t timedout) {
  seastar::rpc::client_options opts;
  opts.send_timeout_data = false;
  return seastar::make_shared<RaftClient>(rpc_, opts, remote_addr);
}

future<> RaftService::OnElectionTimedout(ServerState state) {
  return seastar::with_lock(lock_, [=] {
    ResetElectionTimer();
    LOG_INFO("server={}, state={}, election timedout", serverId_,
             EnumNameServerState(state));
    return ConvertToCandidate();
  });
}

future<> RaftService::start() {
  stopped_ = false;
  ReadPersist();
  // run backgroud, caller should call stop() to sync
  return seastar::do_until(
             [this] { return stopped_; },
             [this] {
               return seastar::with_lock(lock_, [this] { return state_; })
                   .then([this](ServerState state) {
                     auto electionTimeout =
                         electionTimeout_ +
                         ms_t(rand() % electionTimeout_.count());
                     switch (state) {
                     case ServerState::CANDIDATE:
                       (void)LeaderElection();
                     case ServerState::FOLLOWER:
                       return with_timeout(
                           electionTimeout, electionTimer_.get_future(),
                           [this, state](seastar::timed_out_error &e) {
                             return OnElectionTimedout(state);
                           });
                     case ServerState::LEADER:
                       /*return SendHeartBeart().then(
                           [this] { */
                       return seastar::sleep(heartbeatInterval_); /* });*/
                     default:
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

future<> RaftService::LeaderElection() {
  return with_lock(lock_,
                   [this] {
                     return seastar::make_ready_future<ServerState, term_t,
                                                       size_t, size_t>(
                         state_, currentTerm_, LastLogTerm(), LastLogIndex());
                   })
      .then([this](ServerState state, term_t term, term_t llt, size_t lli) {
        if (state_ != ServerState::CANDIDATE) {
          return seastar::make_ready_future();
        }

        auto numVoted = std::make_shared<std::atomic<size_t>>(1);
        return seastar::parallel_for_each(
            peers_.begin(), peers_.end(), [=](auto addr) mutable {
              auto peer = make_client(addr, ms_t(0));
              auto func = rpc_.make_client<future<term_t, id_t, bool>(
                  term_t, id_t, term_t, size_t)>(1);
              auto fut =
                  func(*peer, term, serverId_, llt, lli)
                      .then([this, numVoted, term](term_t rsp_term, id_t addr,
                                                   bool vote) {
                        return seastar::with_lock(lock_, [=] {
                          if (rsp_term > currentTerm_) {
                            LOG_INFO("server={}, receive larger term "
                                     "{}>{}",
                                     serverId_, rsp_term, term);
                            return ConvertToFollwer(rsp_term);
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
                        return peer->stop().finally(
                            [peer] { return seastar::make_ready_future(); });
                      });
              return ignore_rpc_exceptions(std::move(fut));
            });
      });
}

/*
Receiver implementation:
1. Reply false if term < currentTerm (§5.1)
2. If votedFor is null or candidateId, and candidate’s log is at
least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
*/
seastar::future<term_t, id_t, bool> RaftService::RequestVote(term_t term,
                                                             id_t candidateId,
                                                             term_t llt,
                                                             size_t lli) {
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
      return ConvertToFollwer(term).then(fut);
    } else {
      return fut();
    }
  });
}

seastar::future<> RaftService::Persist() const {
  // return seastar::open_file_dma(".raft_meta", seastar::open_flags::create |
  //                                                 seastar::open_flags::wo)
  //     .then([this](auto file) {
  //       auto out = std::make_shared<seastar::output_stream<char>>(
  //           seastar::make_file_output_stream(file));
  //       auto str = seastar::to_sstring(currentTerm_) + " " +
  //                  seastar::to_sstring(votedFor_);
  //       return out->write(str)
  //           .then([out] { return out->flush(); })
  //           .finally([out] { return out->close(); });
  //     });
  return seastar::make_ready_future();
}

void RaftService::ResetElectionTimer() {
  electionTimer_.set_value();
  electionTimer_ = seastar::promise<>();
}

void RaftService::ReadPersist() {
  currentTerm_ = TERMNULL;
  state_ = ServerState::FOLLOWER;
  votedFor_ = VOTENULL;
  log_.emplace_back(std::make_pair(TERMNULL, ""));
}

future<> RaftService::ConvertToCandidate() {
  currentTerm_++;
  LOG_INFO("Convert server({}) state({}=>{}) term({})", serverId_,
           EnumNameServerState(state_),
           EnumNameServerState(ServerState::CANDIDATE), currentTerm_);
  state_ = ServerState::CANDIDATE;
  votedFor_ = serverId_;
  return Persist();
}

future<> RaftService::ConvertToLeader() {
  if (state_ == ServerState::CANDIDATE) {
    LOG_INFO("Convert server({}) state({}=>{}) term {}", serverId_,
             EnumNameServerState(state_),
             EnumNameServerState(ServerState::LEADER), currentTerm_);
    state_ = ServerState::LEADER;
    return Persist();
  }
  return seastar::make_ready_future();
}

future<> RaftService::ConvertToFollwer(term_t term) {
  LOG_INFO("Convert server({}) state({}=>{}) term({} => {})", serverId_,
           EnumNameServerState(state_),
           EnumNameServerState(ServerState::FOLLOWER), currentTerm_, term);
  state_ = ServerState::FOLLOWER;
  currentTerm_ = term;
  votedFor_ = VOTENULL;
  return Persist();
}

bool RaftService::CheckState(ServerState state, term_t term) const {
  return state_ == state && currentTerm_ == term;
}

bool RaftService::CheckLastLog(term_t lastLogTerm, size_t lastLogIndex) const {
  term_t myLastLogTerm = LastLogTerm();
  return lastLogTerm > myLastLogTerm ||
         (lastLogTerm == myLastLogTerm && lastLogIndex >= LastLogIndex());
}

size_t RaftService::LastLogIndex() const { return log_.size() - 1; }

term_t RaftService::LastLogTerm() const {
  size_t lli = LastLogIndex();
  if (lli == 0) {
    return TERMNULL;
  } else {
    return log_[lli].first;
  }
}

future<> RaftService::stop() {
  LOG_INFO("stop server {}", serverId_);
  ResetElectionTimer();
  stopped_ = true;
  return stopped_pro_.get_future();
}

seastar::future<term_t, id_t, bool> RaftService::GetState() {
  LOG_INFO("GetState");
  return seastar::with_lock(lock_, [this] {
    return seastar::make_ready_future<term_t, id_t, bool>(
        currentTerm_, serverId_, state_ == ServerState::LEADER);
  });
}

} // namespace raft
} // namespace laomd
