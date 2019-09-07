#include "core/rpc.h"
#include "echo_impl.h"
using namespace laomd;

int main(int ac, char** av) {
  MyServer my_server;
  EchoServiceImpl echo_service;
  my_server.add(&echo_service);
  my_server.start("127.0.0.1", 6688);
  return 0;
}