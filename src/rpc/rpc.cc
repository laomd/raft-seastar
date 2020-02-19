#include "rpc/rpc.hh"

namespace laomd {

LOG_SETUP(rpc_protocol);

void rpc_server::start() {
  for (auto &service : services_) {
    service->start();
  }
}

seastar::future<> rpc_server::stop() {
  std::deque<seastar::future<>> futs;
  for (auto &service : services_) {
    futs.emplace_back(service->stop());
  }
  return seastar::when_all(futs.begin(), futs.end())
      .then_wrapped(
          [this](auto &&fut) { return rpc_protocol::server::stop(); });
}

} // namespace laomd