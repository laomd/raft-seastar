#include "raft/interface/istate_machine.hh"
#include "util/log.hh"
#include <seastar/net/api.hh>
#include <unordered_map>

namespace laomd {
namespace raft {

class IKVService : public rpc_service {
 public:
  virtual seastar::future<seastar::sstring, bool> get(int index) = 0;
  const char *name() const override { return typeid(*this).name(); }

  uint64_t service_id() const override { return 1; }

  void on_register(rpc_protocol &proto, uint64_t rpc_verb_base) override {
    proto.register_handler(rpc_verb_base + 1,
                           [this](int index) { return get(index); });
  } 
};

class KVStateMachineStub : public IKVService {
  rpc_client client_;
  ms_t timeout_;

public:
  KVStateMachineStub(seastar::rpc::client_options opts,
                     const seastar::ipv4_addr &addr,
                     ms_t timeout);

  virtual ~KVStateMachineStub() = default;

  void start() override {}
  seastar::future<> stop() override { return client_.stop(); }

  seastar::future<seastar::sstring, bool> get(int index) override;
};

class KVStateMachineService : public IKVService, public IStateMachine {
  std::unordered_map<int, LogEntry> entries_;
  LOG_DECLARE();

public:
  explicit KVStateMachineService(const std::string &data_dir);
  virtual ~KVStateMachineService() = default;
  void start() override{};
  seastar::future<> stop() override { return seastar::make_ready_future(); }
  const char *name() const override { return typeid(*this).name(); }
  seastar::future<> apply(const LogEntry &entry) override;
  seastar::future<seastar::sstring, bool> get(int index) override;

private:
  seastar::future<> flush();

private:
  std::string log_file_;
};

} // namespace raft
} // namespace laomd