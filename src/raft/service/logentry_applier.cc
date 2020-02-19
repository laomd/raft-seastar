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

LOG_SETUP(LogEntryApplierService);

seastar::future<> LogEntryApplierService::apply(const LogEntry &entry) {
  entries_[entry.index] = entry;
  return seastar::make_ready_future();
}

} // namespace raft
} // namespace laomd