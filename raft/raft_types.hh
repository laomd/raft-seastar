#pragma once

#include <chrono>

namespace laomd {
namespace raft {

using ms_t = std::chrono::milliseconds;
using term_t = uint64_t;
using id_t = uint64_t;

enum class ServerState { FOLLOWER, CANDIDATE, LEADER };

inline const char *EnumNameServerState(ServerState state) {
  switch (state) {
  case ServerState::FOLLOWER:
    return "FOLLOWER";
  case ServerState::CANDIDATE:
    return "CANDIDATE";
  case ServerState::LEADER:
    return "LEADER";
  default:
    return "UNKNOWN";
  }
}

} // namespace raft
} // namespace laomd