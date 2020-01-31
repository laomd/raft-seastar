#pragma once

#include "raft_types.hh"
#include "rpc/rpc.hh"

namespace laomd {
namespace raft {
struct Raft : public rpc_service {
  virtual uint64_t service_id() const override { return 0; }
  // return currentTerm, serverId and whether granted
  virtual seastar::future<term_t, id_t, bool>
  RequestVote(term_t term, id_t candidateId, term_t llt, size_t lli) = 0;

  // return currentTerm, serverId and whether is leader
  virtual seastar::future<term_t, id_t, bool> GetState() = 0;
};

} // namespace raft
} // namespace laomd