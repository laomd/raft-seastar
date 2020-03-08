#include "raft/interface/istate_machine.hh"
#include "util/log.hh"
#include <seastar/net/api.hh>
#include <unordered_map>

namespace laomd {
namespace raft {

class IKVStateMachine : public IStateMachine {
 public:
  virtual seastar::future<seastar::sstring, bool> get(int index) = 0;
  const char *name() const override { return typeid(*this).name(); }

  void on_register(rpc_protocol &proto, uint64_t rpc_verb_base) override {
    IStateMachine::on_register(proto, rpc_verb_base);
    proto.register_handler(rpc_verb_base + 2,
                           [this](int index) { return get(index); });
  } 
};

class KVStateMachineStub : public IKVStateMachine {
  rpc_client client_;
  ms_t timeout_;

public:
  KVStateMachineStub(seastar::rpc::client_options opts,
                     const seastar::ipv4_addr &addr, ms_t time_out);

  virtual ~KVStateMachineStub() = default;

  void start() override {}
  seastar::future<> stop() override { return client_.stop(); }

  seastar::future<> apply(const LogEntry &entry) override;
  seastar::future<seastar::sstring, bool> get(int index) override;
};

class KVStateMachineService : public IKVStateMachine {
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