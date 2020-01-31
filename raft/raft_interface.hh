#pragma once

#include "raft/raft_serializer.hh"
#include "rpc/rpc.hh"
#include <vector>

namespace laomd {
namespace raft {
struct Raft : public rpc_service {
  virtual uint64_t service_id() const override { return 0; }
  // return currentTerm, serverId and whether granted
  virtual seastar::future<term_t, id_t, bool>
  RequestVote(term_t term, id_t candidateId, term_t llt, int lli) = 0;

  // return currentTerm, Success
  virtual seastar::future<term_t, bool>
  AppendEntries(term_t term, id_t leaderId, term_t plt, int pli,
                const std::vector<LogEntry> &entries, int leaderCommit) = 0;

  // return currentTerm, serverId and whether is leader
  virtual seastar::future<term_t, id_t, bool> GetState() = 0;

  virtual void on_register(rpc_protocol &proto,
                           uint64_t rpc_verb_base) override {
    proto.register_handler(
        rpc_verb_base + 1,
        [this](term_t term, id_t candidateId, term_t llt, int lli) {
          return RequestVote(term, candidateId, llt, lli);
        });
    proto.register_handler(
        rpc_verb_base + 2,
        [this](term_t term, id_t leaderId, term_t plt, int pli,
               std::vector<LogEntry> entries, int leaderCommit) {
          return AppendEntries(term, leaderId, plt, pli, entries, leaderCommit);
        });
    proto.register_handler(rpc_verb_base + 3, [this] { return GetState(); });
  }
};

} // namespace raft
} // namespace laomd