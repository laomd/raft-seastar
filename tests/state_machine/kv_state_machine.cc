#include "kv_state_machine.hh"
#include <filesystem>
#include <fstream>

namespace laomd {
namespace raft {

KVStateMachineStub::KVStateMachineStub(seastar::rpc::client_options opts,
                                         const seastar::ipv4_addr &addr,
                                         ms_t time_out)
    : client_(std::move(opts), addr), timeout_(time_out) {}

seastar::future<> KVStateMachineStub::apply(const LogEntry &entry) {
  auto func = client_.get_handler<seastar::future<>(LogEntry)>(*this, 1);
  return func(client_, timeout_, entry);
}

seastar::future<seastar::sstring, bool> KVStateMachineStub::get(int index) {
  auto func = client_.get_handler<seastar::future<seastar::sstring, bool>(int)>(
      *this, 2);
  return func(client_, timeout_, index);
}

LOG_SETUP(KVStateMachineService);

KVStateMachineService::KVStateMachineService(const std::string &data_dir) {
  std::filesystem::path log_dir(data_dir);
  log_dir /= "log";
  std::filesystem::create_directories(log_dir);
  log_file_ = (log_dir / "entry").string();
}

seastar::future<> KVStateMachineService::apply(const LogEntry &entry) {
  LOG_INFO("apply log {}", entry);
  entries_[entry.index] = entry;
  return seastar::make_ready_future();
}

seastar::future<seastar::sstring, bool> KVStateMachineService::get(int index) {
  auto it = entries_.find(index);
  if (it != entries_.end()) {
    return seastar::make_ready_future<seastar::sstring, bool>(it->second.log,
                                                              true);
  } else {
    return seastar::make_ready_future<seastar::sstring, bool>("", false);
  }
}

} // namespace raft
} // namespace laomd