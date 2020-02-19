#include "raft/service/logentry_applier.hh"
#include <fstream>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
using namespace seastar;
using namespace laomd;
using namespace laomd::raft;
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
  app_template app;
  app.add_options()("addr,a", bpo::value<std::string>()->required(),
                    "server addr")("log-file,l", bpo::value<std::string>(),
                                   "log file, default is stdout");
  std::unique_ptr<rpc_server> server;
  std::ofstream fout;
  app.run_deprecated(argc, argv, [&] {
    auto &&cfg = app.configuration();
    if (cfg.count("log-file")) {
      std::string log_file = cfg["log-file"].as<std::string>();
      if (log_file != "stdout") {
        fout.open(log_file);
        seastar::logger::set_ostream(fout);
        engine().at_exit([&fout] {
          fout.close();
          return seastar::make_ready_future();
        });
      }
    }

    server = std::make_unique<rpc_server>(
        seastar::ipv4_addr(cfg["addr"].as<std::string>()));
    server->register_service<LogEntryApplierService>();
    engine().at_exit([&server] { return server->stop(); });
    server->start();
  });
}
