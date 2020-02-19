#pragma once

#include "raft/interface/iraft.hh"
#include "raft/raft_client.hh"

namespace laomd {
namespace raft {

class RaftService : public IRaft {
public:
  seastar::lw_shared_ptr<RaftClient>
  make_client(const seastar::ipv4_addr &remote_addr, ms_t time_out) {
    seastar::rpc::client_options opts;
    opts.send_timeout_data = false;
    return seastar::make_lw_shared<RaftClient>(opts, remote_addr, time_out);
  }
};

} // namespace raft
} // namespace laomd