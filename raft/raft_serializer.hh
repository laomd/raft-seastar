#pragma once
#include "raft/raft_types.hh"
#include "rpc/serializer.hh"

namespace laomd {
namespace raft {

template <typename Output>
inline void write(rpc_serializer s, Output &out, const LogEntry &v) {
  write(s, out, v.term);
  write(s, out, v.index);
  write(s, out, v.log);
}

template <typename Input>
inline LogEntry read(rpc_serializer s, Input &in, rpc::type<LogEntry>) {
  LogEntry entry;
  entry.term = read(s, in, rpc::type<term_t>());
  entry.index = read(s, in, rpc::type<int>());
  entry.log = read(s, in, rpc::type<seastar::sstring>());
  return entry;
}

} // namespace raft
} // namespace laomd