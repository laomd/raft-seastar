#include "raft/service/logentry_applier.hh"
#include "raft/service/raft_impl.hh"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <fstream>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
using namespace seastar;
using namespace std::chrono;
using namespace laomd;
using namespace laomd::raft;
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
  app_template app;
  app.add_options()("me,i", bpo::value<int>()->required(), "my peer index")(
      "electionTimedout,e", bpo::value<int>()->default_value(100))(
      "heartbeatInterval,b", bpo::value<int>()->default_value(10))(
      "log-file,l", bpo::value<std::string>(), "log file, default is stdout")(
      "applier,a", bpo::value<std::string>()->required(),
      "log entry applier addr")(
      "peers,p", bpo::value<std::string>()->required(), "all peers");
  std::unique_ptr<rpc_server> server;
  std::unique_ptr<LogEntryApplierStub> log_applier;
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
    std::vector<std::string> peers;
    boost::split(peers, cfg["peers"].as<std::string>(), boost::is_any_of(","),
                 boost::token_compress_on);
    auto me = cfg["me"].as<int>();

    seastar::rpc::client_options opts;
    opts.send_timeout_data = false;
    log_applier = std::make_unique<LogEntryApplierStub>(
        std::move(opts), cfg["applier"].as<std::string>(),
        ms_t(std::numeric_limits<uint32_t>::max()));

    server = std::make_unique<rpc_server>(seastar::ipv4_addr(peers[me]));
    server->register_service<RaftImpl>(
        me, peers, ms_t(cfg["electionTimedout"].as<int>()),
        ms_t(cfg["heartbeatInterval"].as<int>()), log_applier.get());
    engine().at_exit([&server] { return server->stop(); });
    engine().at_exit([&log_applier] { return log_applier->stop(); });
    server->start();
  });
}
