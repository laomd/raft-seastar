#pragma once

#include "raft/interface/raft_serializer.hh"
#include "rpc/rpc.hh"
#include <seastar/core/future.hh>

namespace laomd {
namespace raft {
class IStateMachine {
public:
  virtual seastar::future<> apply(const LogEntry &entry) = 0;
};

} // namespace raft
} // namespace laomd