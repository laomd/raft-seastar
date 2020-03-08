#pragma once

#include "raft/interface/raft_serializer.hh"
#include "rpc/rpc.hh"
#include <seastar/core/future.hh>

namespace laomd {
namespace raft {
class IStateMachine : public rpc_service {
public:
  virtual seastar::future<> apply(const LogEntry &entry) = 0;

  void on_register(rpc_protocol &proto, uint64_t rpc_verb_base) override {
    proto.register_handler(rpc_verb_base + 1,
                           [this](LogEntry entry) { return apply(entry); });
  }
  uint64_t service_id() const override { return 1; }
};

} // namespace raft
} // namespace laomd