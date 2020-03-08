#pragma once

#include <chrono>
#include <seastar/core/sstring.hh>
#include <sstream>

namespace laomd {
namespace raft {

using ms_t = std::chrono::milliseconds;
using term_t = int;
using id_t = int;

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

struct LogEntry {
  term_t term;
  int index;
  seastar::sstring log;
};

inline std::ostream &operator<<(std::ostream &out, const LogEntry &entry) {
  return out << entry.term << " " << entry.index << " " << entry.log;
}

inline std::istream &operator>>(std::istream &in, LogEntry &entry) {
  return in >> entry.term >> entry.index >> entry.log;
}

} // namespace raft
} // namespace laomd

namespace std {
inline std::string to_string(const laomd::raft::LogEntry &entry) {
  std::ostringstream out;
  out << "(term=" << entry.term << ",index=" << entry.index
      << ",log=" << entry.log << ")";
  return out.str();
}
} // namespace std
