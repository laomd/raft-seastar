#pragma once

namespace laomd {
namespace raft {

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