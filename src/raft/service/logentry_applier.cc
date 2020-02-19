#include "raft/service/logentry_applier.hh"

namespace laomd {
namespace raft {

LogEntryApplierStub::LogEntryApplierStub(seastar::rpc::client_options opts,
                    const seastar::ipv4_addr &addr, ms_t time_out)
    : client_(std::move(opts), addr), timeout_(time_out) {}

seastar::future<> LogEntryApplierStub::apply(const LogEntry &entry) {
  auto func = client_.get_handler<seastar::future<>(LogEntry)>(*this, 1);
  return func(client_, timeout_, entry);
}

seastar::future<seastar::sstring, bool> LogEntryApplierStub::get(int index) {
  auto func = client_.get_handler<seastar::future<seastar::sstring, bool>(int)>(*this, 2);
  return func(client_, timeout_, index);
}

LOG_SETUP(LogEntryApplierService);

seastar::future<> LogEntryApplierService::apply(const LogEntry &entry) {
  LOG_INFO("appling log {}", entry);
  entries_[entry.index] = entry;
  return seastar::make_ready_future();
}

seastar::future<seastar::sstring, bool> LogEntryApplierService::get(int index) {
  auto it = entries_.find(index);
  if (it != entries_.end()) {
    return seastar::make_ready_future<seastar::sstring, bool>(it->second.log, true);
  } else {
    return seastar::make_ready_future<seastar::sstring, bool>("", false);
  }
}

} // namespace raft
} // namespace laomd