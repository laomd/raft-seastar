#include <lao_rpc/rpc.hh>
#include "echo_impl.h"
using namespace laomd;

int main(int ac, char** av) {
  FLAGS_logtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  google::InitGoogleLogging(av[0]);
  RpcServer my_server;
  EchoServiceImpl echo_service;
  my_server.add(&echo_service);
  my_server.start("127.0.0.1", 6688);
  return 0;
}