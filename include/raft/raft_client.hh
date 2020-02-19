#pragma once

#include "raft/interface/iraft.hh"

namespace laomd {
namespace raft {
class RaftClient : public IRaft {
  rpc_client client_;
  ms_t timeout_;

public:
  RaftClient(seastar::rpc::client_options opts, const seastar::ipv4_addr &addr,
             ms_t time_out)
      : client_(opts, addr), timeout_(time_out) {}
  virtual ~RaftClient() = default;

  virtual void start() override {}
  virtual seastar::future<> stop() override { return client_.stop(); }

  virtual seastar::future<term_t, id_t, bool>
  RequestVote(term_t term, id_t candidateId, term_t llt, int lli) override {
    auto func = client_.get_handler<seastar::future<term_t, id_t, bool>(
        term_t, id_t, term_t, int)>(*this, 1);
    return func(client_, timeout_, term, candidateId, llt, lli);
  }

  // return currentTerm, Success
  virtual seastar::future<term_t, bool>
  AppendEntries(term_t term, id_t leaderId, term_t plt, int pli,
                const std::vector<LogEntry> &entries,
                int leaderCommit) override {
    auto func = client_.get_handler<seastar::future<term_t, bool>(
        term_t, id_t, term_t, int, std::vector<LogEntry>, int)>(*this, 2);
    return func(client_, timeout_, term, leaderId, plt, pli, entries,
                leaderCommit);
  }

  // return currentTerm, serverId and whether is leader
  virtual seastar::future<term_t, id_t, bool> GetState() override {
    auto func =
        client_.get_handler<seastar::future<term_t, id_t, bool>()>(*this, 3);
    return func(client_, timeout_);
  }

  // return index, ok
  virtual seastar::future<int, bool>
  Append(const seastar::sstring &cmd) override {
    auto func =
        client_.get_handler<seastar::future<int, bool>(seastar::sstring)>(*this,
                                                                          4);
    return func(client_, timeout_, cmd);
  }
};

} // namespace raft
} // namespace laomd