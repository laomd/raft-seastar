#pragma once

#include "raft/raft_client.hh"
#include "raft/raft_interface.hh"

namespace laomd {
namespace raft {

class RaftService : public Raft {
public:
  seastar::lw_shared_ptr<RaftClient>
  make_client(const seastar::ipv4_addr &remote_addr, ms_t time_out) {
    seastar::rpc::client_options opts;
    opts.send_timeout_data = false;
    return seastar::make_lw_shared<RaftClient>(opts, remote_addr, time_out);
  }

  virtual void on_register(rpc_protocol &proto,
                           uint64_t rpc_verb_base) override {
    proto.register_handler(
        rpc_verb_base + 1,
        [this](term_t term, id_t candidateId, term_t llt, size_t lli) {
          return RequestVote(term, candidateId, llt, lli);
        });
    proto.register_handler(rpc_verb_base + 3, [this] { return GetState(); });
  }
};

} // namespace raft
} // namespace laomd