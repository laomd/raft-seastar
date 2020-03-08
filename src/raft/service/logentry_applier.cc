#include "raft/service/logentry_applier.hh"
#include <filesystem>
#include <seastar/core/coroutine.hh>
#include <seastar/core/fstream.hh>
#include <fstream>

namespace laomd {
namespace raft {

LogEntryApplierStub::LogEntryApplierStub(seastar::rpc::client_options opts,
                                         const seastar::ipv4_addr &addr,
                                         ms_t time_out)
    : client_(std::move(opts), addr), timeout_(time_out) {}

seastar::future<> LogEntryApplierStub::apply(const LogEntry &entry) {
  auto func = client_.get_handler<seastar::future<>(LogEntry)>(*this, 1);
  return func(client_, timeout_, entry);
}

seastar::future<seastar::sstring, bool> LogEntryApplierStub::get(int index) {
  auto func = client_.get_handler<seastar::future<seastar::sstring, bool>(int)>(
      *this, 2);
  return func(client_, timeout_, index);
}

LOG_SETUP(LogEntryApplierService);

LogEntryApplierService::LogEntryApplierService(const std::string &data_dir) {
  std::filesystem::path log_dir(data_dir);
  log_dir /= "log";
  std::filesystem::create_directories(log_dir);
  log_file_ = (log_dir / "entry").string();
}

void LogEntryApplierService::start() {
  LOG_INFO("logentry file {}", log_file_);
  // restore log entries from log file
  std::ifstream fin(log_file_);
  LogEntry entry;
  while (fin >> entry) {
    entries_.emplace(entry.index, entry);
  }
}

seastar::future<> LogEntryApplierService::apply(const LogEntry &entry) {
  LOG_INFO("apply log {}", entry);
  entries_[entry.index] = entry;
  return seastar::make_ready_future();
}

seastar::future<> LogEntryApplierService::flush() {
  std::ofstream fout(log_file_ + ".tmp");
  for (auto&& item: entries_) {
    auto& entry = item.second;
    fout << entry.index << ' ' << entry.term << ' ' << entry.log << std::endl;
  }
  fout.close();
  std::filesystem::remove(log_file_);
  std::filesystem::rename(log_file_ + ".tmp", log_file_);
  return seastar::make_ready_future();
}

seastar::future<seastar::sstring, bool> LogEntryApplierService::get(int index) {
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