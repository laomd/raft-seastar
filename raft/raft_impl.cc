#include "raft_impl.hh"
#include "common_generated.h"
#include "service_generated.h"
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
  for (auto &&server : peers) {
    smf::rpc_client_opts opts;
    opts.server_addr = server;
    peers_.emplace_back(seastar::make_shared<RaftClient>(std::move(opts)));
  }
  ReadPersist();
  Start();
}

future<> RaftImpl::Start() {
  return seastar::do_until(
      [this] { return stopped_; },
      [this] {
        return seastar::with_lock(lock_, [this] { return state_; })
            .then([this](ServerState state) {
              auto electionTimeout =
                  electionTimeout_ + ms_t(rand() % electionTimeout_.count());
              switch (state) {
              case ServerState_FOLLOWER:
                electionTimer_.set_callback([this] {
                  return seastar::with_lock(lock_, [this] {
                    LOG_INFO("election timedout");
                    return ConvertToCandidate();
                  });
                });
                electionTimer_.arm(electionTimeout);
                return seastar::sleep(electionTimeout);
              case ServerState_CANDIDATE:
                return seastar::with_timeout(clock_type::now() +
                                                 electionTimeout,
                                             LeaderElection())
                    .handle_exception(ignore_exception);
              case ServerState_LEADER:
                return SendHeartBeart().then(
                    [this] { return seastar::sleep(heartbeatInterval_); });
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
          return peer->reconnect().then_wrapped([=](future<> &&fut) {
            if (fut.failed()) {
              LOG_INFO("failed to connect to {}", peer->server_addr);
              return fut.handle_exception(ignore_exception);
            } else {
              smf::rpc_typed_envelope<VoteRequest> req;
              req.data->term = term;
              req.data->candidateId = serverId_;
              req.data->lastLogTerm = llt;
              req.data->lastLogIndex = lli;
              using RspType =
                  smf::rpc_recv_typed_context<laomd::raft::VoteResponse>;
              return seastar::with_timeout(
                         clock_type::now() + electionTimeout_ / peers_.size(),
                         peer->RequestVote(req.serialize_data()))
                  .then([this, peer, numVoted](RspType &&rsp) {
                    return seastar::with_lock(
                        lock_, [this, addr = peer->server_addr, numVoted,
                                term = rsp->term(), vote = rsp->voteGranted()] {
                          if (term > currentTerm_) {
                            return ConvertToFollwer(term);
                          }
                          if (!CheckState(ServerState_CANDIDATE, term)) {
                            return seastar::make_ready_future();
                          }
                          if (vote) {
                            (*numVoted)++;
                            LOG_INFO("vote from {}", addr);
                          }
                          if (*numVoted > (peers_.size() + 1) / 2) {
                            LOG_INFO("Server({}) win vote", serverId_);
                            return ConvertToLeader();
                          }
                          return seastar::make_ready_future();
                        });
                  })
                  .handle_exception(ignore_exception);
            }
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
            LOG_INFO("{} vote {}, term:{}, candidate term:{}", serverId_,
                     votedFor_, currentTerm_, term);
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
          LOG_INFO("receive heartbeart from server {}", leaderId);
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
    return seastar::do_for_each(
        peers_.begin(), peers_.end(), [this](auto &&peer) {
          return peer->reconnect().then_wrapped([=](future<> &&fut) {
            if (fut.failed()) {
              LOG_INFO("failed to connect to {}", peer->server_addr);
              return fut.handle_exception(ignore_exception);
            } else {
              smf::rpc_typed_envelope<AppendEntriesReq> req;
              req.data->term = currentTerm_;
              req.data->leaderId = serverId_;
              using RspType = smf::rpc_recv_typed_context<AppendEntriesRsp>;
              return seastar::with_timeout(
                         clock_type::now() + heartbeatInterval_ / peers_.size(),
                         peer->AppendEntries(req.serialize_data())
                             .discard_result())
                  .handle_exception(ignore_exception);
            }
          });
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

void RaftImpl::ResetElectionTimer() {
  electionTimer_.set_callback(do_nothing);
  electionTimer_.cancel();
}

void RaftImpl::ReadPersist() {}

future<> RaftImpl::ConvertToCandidate() {
  ResetElectionTimer();
  currentTerm_++;
  LOG_INFO("Convert server({}) state({}=>candidate) term({})", serverId_,
           EnumNameServerState(state_), currentTerm_);
  state_ = ServerState_CANDIDATE;
  votedFor_ = serverId_;
  return Persist();
}

future<> RaftImpl::ConvertToLeader() {
  if (state_ == ServerState_CANDIDATE) {
    ResetElectionTimer();
    LOG_INFO("Convert server({}) state({}=>leader) term {}", serverId_,
             EnumNameServerState(state_), currentTerm_);
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
  LOG_INFO("Convert server({}) state({}=>follower) term({} => {})", serverId_,
           EnumNameServerState(state_), currentTerm_, term);
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

future<> RaftImpl::Stop() {
  LOG_INFO("Kill Server({})", serverId_);
  ResetElectionTimer();
  return seastar::make_ready_future();
}

future<std::pair<term_t, bool>> RaftImpl::GetState() {
  return seastar::with_lock(lock_, [this] {
    return std::make_pair(currentTerm_, state_ == ServerState_LEADER);
  });
}

} // namespace raft
} // namespace laomd