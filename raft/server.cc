#include "raft/raft_impl.hh"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
using namespace seastar;
using namespace std::chrono;
using namespace laomd::raft;
namespace bpo = boost::program_options;

int main(int argc, char **argv) {
  app_template::config cfg;
  cfg.auto_handle_sigint_sigterm = false;
  app_template app(cfg);
  app.add_options()("me,i", bpo::value<int>()->required(), "my peer index")(
      "electionTimedout,e", bpo::value<int>()->default_value(100))(
      "heartbeatInterval,b", bpo::value<int>()->default_value(10))(
      "peers,p", bpo::value<std::string>()->required(), "all peers");
  std::unique_ptr<RaftService> server;
  app.run_deprecated(argc, argv, [&] {
    auto &&cfg = app.configuration();
    std::vector<std::string> peers;
    boost::split(peers, cfg["peers"].as<std::string>(), boost::is_any_of(","),
                 boost::token_compress_on);
    auto me = cfg["me"].as<int>();

    server = std::make_unique<RaftService>(
        me, peers, ms_t(cfg["electionTimedout"].as<int>()),
        ms_t(cfg["heartbeatInterval"].as<int>()));
    server->start();
    engine().at_exit([&server] { return server->stop(); });
  });
}
