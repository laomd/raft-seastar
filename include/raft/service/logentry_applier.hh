#include "raft/interface/ilog_applier.hh"
#include "util/log.hh"
#include <seastar/net/api.hh>
#include <unordered_map>

namespace laomd {
namespace raft {
class LogEntryApplierStub : public ILogApplier {
  rpc_client client_;
  ms_t timeout_;

public:
  LogEntryApplierStub(seastar::rpc::client_options opts,
                      const seastar::ipv4_addr &addr, ms_t time_out);

  virtual ~LogEntryApplierStub() = default;

  void start() override {}
  seastar::future<> stop() override { return client_.stop(); }

  seastar::future<> apply(const LogEntry &entry) override;
  seastar::future<seastar::sstring, bool> get(int index) override;
};

class LogEntryApplierService : public ILogApplier {
  std::unordered_map<int, LogEntry> entries_;
  LOG_DECLARE();

public:
  explicit LogEntryApplierService(const std::string &data_dir);
  virtual ~LogEntryApplierService() = default;
  void start() override;
  seastar::future<> stop() override { return flush(); }
  seastar::future<> apply(const LogEntry &entry) override;
  seastar::future<seastar::sstring, bool> get(int index) override;

private:
  seastar::future<> flush();

private:
  std::string log_file_;
};

} // namespace raft
} // namespace laomd