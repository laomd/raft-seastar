#include "echo_impl.h"
using namespace grpc;

int main(int ac, char** av) {
  EchoServiceImpl echo_service;
  ServerBuilder oBuilder;
  oBuilder.AddListeningPort("127.0.0.1:6688", grpc::InsecureServerCredentials());
  oBuilder.RegisterService(&echo_service);

  std::unique_ptr<Server> server(oBuilder.BuildAndStart());
  server->Wait();
  return 0;
}