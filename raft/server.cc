#include "raft/raft_impl.hh"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
using namespace seastar;
using namespace std::chrono;
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
  app_template::config cfg;
  cfg.auto_handle_sigint_sigterm = false;
  app_template app(cfg);
  app.add_options()
      // ("peers,p", bpo::value<std::string>()->required(), "all peers")
      ("me,i", bpo::value<int>()->required(), "my peer index");
  std::unique_ptr<laomd::raft::RaftService> server;
  app.run_deprecated(argc, argv, [&] {
    auto &&cfg = app.configuration();
    std::vector<std::string> peers;
    boost::split(peers, "0.0.0.0:12000,0.0.0.0:12001,0.0.0.0:12002",
                 boost::is_any_of(","), boost::token_compress_on);
    auto me = cfg["me"].as<int>();

    server = std::make_unique<laomd::raft::RaftService>(me, peers, 100ms, 10ms);
    server->start();
    engine().at_exit([&server] { return server->stop(); });
  });
}
