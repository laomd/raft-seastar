#include <raft/raft_impl.hh>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/net/socket_defs.hh>
#include <smf/log.h>
#include <smf/rpc_server.h>
#include <string>
#include <vector>
using namespace laomd;
namespace po = boost::program_options;

int main(int ac, char **av) {
  seastar::app_template app;
  app.add_options()("port,p", po::value<uint16_t>()->default_value(12000), "port for service")(
      "others,o", po::value<std::vector<std::string>>()->multitoken(), "other servers");
  std::shared_ptr<smf::rpc_server> rpc;

  return app.run_deprecated(ac, av, [&] {
    auto &cfg = app.configuration();

    smf::rpc_server_args args;
    args.rpc_port = cfg["port"].as<uint16_t>();
    args.http_port = args.rpc_port + 10000;
    rpc = std::make_shared<smf::rpc_server>(args);

    std::vector<seastar::ipv4_addr> others;
    for (auto&& s: cfg["others"].as<std::vector<std::string>>()) {
      others.emplace_back(s);
    }
    rpc->register_service<raft::RaftImpl>(args.rpc_port, others);
    rpc->start();

    seastar::engine().at_exit([rpc] {
      return rpc->stop();
    });

  });
}