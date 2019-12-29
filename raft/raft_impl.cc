#include "raft_impl.hh"
#include "raft/common.smf.fb.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <lao_utils/function.hh>
#include <memory>
#include <seastar/core/fstream.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/lowres_clock.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/core/sleep.hh>
#include <smf/log.h>
#include <smf/rpc_client.h>
#include <utility>

namespace laomd {
namespace raft {

RaftImpl::RaftImpl(id_t serverId, const std::vector<seastar::ipv4_addr> &peers,
                   ms_t electionTimeout, ms_t heartbeatInterval)
    : Raft(), state_(ServerState_FOLLOWER), stopped_(false),
      electionTimeout_(electionTimeout), heartbeatInterval_(heartbeatInterval),
      serverId_(serverId), currentTerm_(TERMNULL), votedFor_(VOTENULL) {
  for (int i = 0; i < peers.size(); i++) {
    if (i != serverId) {
      smf::rpc_client_opts opts;
      opts.server_addr = peers[i];
      peers_.emplace_back(seastar::make_shared<RaftClient>(std::move(opts)));
    }
  }
}

void RaftImpl::start() {
  Raft::start();
  ReadPersist();
  ResetElectionTimer();
  (void)seastar::do_until(
      [this] { return stopped_; },
      [this] {
        return seastar::with_lock(lock_, [this] { return state_; })
            .then([this](ServerState state) {
              auto electionTimeout =
                  electionTimeout_ + ms_t(rand() % electionTimeout_.count());
              switch (state) {
              case ServerState_FOLLOWER:
                timerStop_ = false;
                return with_timeout(
                    electionTimeout,
                    seastar::do_until(
                        [this] {
                          return timerStop_;
                        }, // maybe race condition without lock, but it doesnot
                           // matter
                        [] { return seastar::make_ready_future(); }),
                    [this](seastar::timed_out_error &) {
                      return seastar::with_lock(lock_, [this] {
                        LOG_INFO("server={}, election timedout", serverId_);
                        return ConvertToCandidate();
                      });
                    });
              case ServerState_CANDIDATE:
                return with_timeout(electionTimeout, LeaderElection())
                    .finally([this] {
                      return seastar::with_lock(lock_, [this] {
                        if (state_ == ServerState_CANDIDATE) {
                          return ConvertToCandidate();
                        }
                        return seastar::make_ready_future();
                      });
                    });
              case ServerState_LEADER:
                /*return SendHeartBeart().then(
                    [this] { */
                return seastar::sleep(heartbeatInterval_); /* });*/
              default:
                return seastar::make_ready_future();
              }
            });
      });
}

future<> RaftImpl::LeaderElection() {
  return lock_.lock().then([this] {
    if (state_ != ServerState_CANDIDATE) {
      lock_.unlock();
      return seastar::make_ready_future();
    }
    term_t term = currentTerm_, llt = LastLogTerm();
    size_t lli = LastLogIndex();
    lock_.unlock();

    auto numVoted = std::make_shared<std::atomic<size_t>>(1);
    return seastar::parallel_for_each(
        peers_.begin(), peers_.end(),
        [this, term, llt, lli, numVoted](auto peer) mutable {
          return peer->connect()
              .then([=] {
                smf::rpc_typed_envelope<VoteRequest> req;
                req.data->term = term;
                req.data->candidateId = serverId_;
                req.data->lastLogTerm = llt;
                req.data->lastLogIndex = lli;
                using RspType =
                    smf::rpc_recv_typed_context<laomd::raft::VoteResponse>;
                auto fut =
                    peer->RequestVote(req.serialize_data())
                        .then([this, peer, numVoted](RspType &&rsp) {
                          return seastar::with_lock(
                              lock_,
                              [this, addr = peer->server_addr, numVoted,
                               term = rsp->term(), vote = rsp->voteGranted()] {
                                if (term > currentTerm_) {
                                  return ConvertToFollwer(term);
                                }
                                if (!CheckState(ServerState_CANDIDATE, term)) {
                                  return seastar::make_ready_future();
                                }
                                if (vote) {
                                  (*numVoted)++;
                                  LOG_INFO("server={}, vote from {}", serverId_,
                                           addr);
                                }
                                if (*numVoted > (peers_.size() + 1) / 2) {
                                  LOG_INFO("server({}) win vote", serverId_);
                                  return ConvertToLeader();
                                }
                                return seastar::make_ready_future();
                              });
                        })
                    /*.handle_exception_type(
                        ignore_exception<smf::remote_connection_error>)*/
                    ;
                return with_timeout(electionTimeout_, std::move(fut));
              })
              .handle_exception_type(ignore_exception<std::system_error>)
              .handle_exception_type(
                  ignore_exception<smf::remote_connection_error>)
              .handle_exception(
                  [this](auto e) { LOG_WARN("unexpected exception {}", e); });
        });
  });
}

/*
Receiver implementation:
1. Reply false if term < currentTerm (§5.1)
2. If votedFor is null or candidateId, and candidate’s log is at
least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
*/
seastar::future<smf::rpc_typed_envelope<VoteResponse>>
RaftImpl::RequestVote(smf::rpc_recv_typed_context<VoteRequest> &&rec) {
  using RspType = smf::rpc_typed_envelope<VoteResponse>;
  using ReqType = smf::rpc_typed_envelope<VoteRequest>;
  return seastar::with_lock(
      lock_, [this, term = rec->term(), candidateId = rec->candidateId(),
              llt = rec->lastLogTerm(), lli = rec->lastLogIndex()] {
        auto fut = [=] {
          RspType rsp;
          rsp.envelope.set_status(200);
          rsp.data->voteGranted = false;
          if (term == currentTerm_ &&
              (votedFor_ == VOTENULL || votedFor_ == candidateId) &&
              CheckLastLog(llt, lli)) {
            rsp.data->voteGranted = true;
            votedFor_ = candidateId;
            state_ = ServerState_FOLLOWER;
            ResetElectionTimer();
            LOG_INFO("server={}, vote {}, term:{}, candidate term:{}",
                     serverId_, votedFor_, currentTerm_, term);
          }
          rsp.data->term = currentTerm_;
          return seastar::make_ready_future<RspType>(std::move(rsp));
        };
        if (term > currentTerm_) {
          return ConvertToFollwer(term).then(fut);
        } else {
          return fut();
        }
      });
}

seastar::future<smf::rpc_typed_envelope<AppendEntriesRsp>>
RaftImpl::AppendEntries(smf::rpc_recv_typed_context<AppendEntriesReq> &&rec) {
  using RspType = smf::rpc_typed_envelope<AppendEntriesRsp>;
  return seastar::with_lock(lock_, [this] { return currentTerm_; })
      .then([this, rec_term = rec->term(),
             leaderId = rec->leaderId()](term_t term) {
        if (rec_term < term) {
          RspType rsp;
          rsp.envelope.set_status(200);
          return seastar::make_ready_future<RspType>(std::move(rsp));
        } else {
          LOG_DEBUG("server={}, receive heartbeart from leader {}", serverId_,
                    leaderId);
          return ConvertToFollwer(rec_term).then([this] {
            RspType rsp;
            rsp.envelope.set_status(200);
            return seastar::make_ready_future<RspType>(std::move(rsp));
          });
        }
      });
}

future<> RaftImpl::SendHeartBeart() const {
  return seastar::with_lock(lock_, [this] {
    return seastar::parallel_for_each(
        peers_.begin(), peers_.end(), [this](auto &&peer) {
          return peer->connect()
              .then([=] {
                smf::rpc_typed_envelope<AppendEntriesReq> req;
                req.data->term = currentTerm_;
                req.data->leaderId = serverId_;
                using RspType = smf::rpc_recv_typed_context<AppendEntriesRsp>;
                return with_timeout(
                    heartbeatInterval_,
                    peer->AppendEntries(req.serialize_data()).discard_result());
              })
              .handle_exception_type(ignore_exception<std::system_error>)
              .handle_exception_type(
                  ignore_exception<smf::remote_connection_error>)
              .handle_exception(
                  [this](auto e) { LOG_WARN("unexpected exception {}", e); });
        });
  });
}

seastar::future<> RaftImpl::Persist() const {
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

void RaftImpl::ResetElectionTimer() { timerStop_ = true; }

void RaftImpl::ReadPersist() {}

future<> RaftImpl::ConvertToCandidate() {
  ResetElectionTimer();
  currentTerm_++;
  LOG_INFO("Convert server({}) state({}=>{}) term({})", serverId_,
           EnumNameServerState(state_),
           EnumNameServerState(ServerState_CANDIDATE), currentTerm_);
  state_ = ServerState_CANDIDATE;
  votedFor_ = serverId_;
  return Persist();
}

future<> RaftImpl::ConvertToLeader() {
  if (state_ == ServerState_CANDIDATE) {
    ResetElectionTimer();
    LOG_INFO("Convert server({}) state({}=>{}) term {}", serverId_,
             EnumNameServerState(state_),
             EnumNameServerState(ServerState_LEADER), currentTerm_);
    state_ = ServerState_LEADER;
    return Persist();
  }
  return seastar::make_ready_future();
}

future<> RaftImpl::ConvertToFollwer(term_t term) {
  ResetElectionTimer();
  if (state_ == ServerState_FOLLOWER && term == currentTerm_) {
    return seastar::make_ready_future();
  }
  LOG_INFO("Convert server({}) state({}=>{}) term({} => {})", serverId_,
           EnumNameServerState(state_),
           EnumNameServerState(ServerState_FOLLOWER), currentTerm_, term);
  state_ = ServerState_FOLLOWER;
  currentTerm_ = term;
  votedFor_ = VOTENULL;
  return Persist();
}

bool RaftImpl::CheckState(ServerState state, term_t term) const {
  return state_ == state && currentTerm_ == term;
}

bool RaftImpl::CheckLastLog(term_t lastLogTerm, size_t lastLogIndex) const {
  term_t myLastLogTerm = LastLogTerm();
  return lastLogTerm > myLastLogTerm ||
         (lastLogTerm == myLastLogTerm && lastLogIndex >= LastLogIndex());
}

size_t RaftImpl::LastLogIndex() const { return log_.size(); }

term_t RaftImpl::LastLogTerm() const {
  size_t lli = LastLogIndex();
  if (lli == 0) {
    return TERMNULL;
  } else {
    return log_[lli].term();
  }
}

future<> RaftImpl::stop() {
  LOG_INFO("stop server {}", serverId_);
  ResetElectionTimer();
  stopped_ = true;
  std::vector<future<>> futs;
  for (auto &&client : peers_) {
    futs.emplace_back(client->stop());
  }
  return seastar::when_all(futs.begin(), futs.end()).then([this](auto &&) {
    return Raft::stop();
  });
}

seastar::future<smf::rpc_typed_envelope<GetStateRsp>>
RaftImpl::GetState(smf::rpc_recv_typed_context<GetStateReq> &&rec) {
  return seastar::with_lock(lock_, [this] {
    smf::rpc_typed_envelope<GetStateRsp> rsp;
    rsp.envelope.set_status(200);
    rsp.data->term = currentTerm_;
    rsp.data->serverId = serverId_;
    rsp.data->isLeader = state_ == ServerState_LEADER;
    return rsp;
  });
}

} // namespace raft
} // namespace laomd
