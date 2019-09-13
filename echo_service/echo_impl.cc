#include "echo_impl.h"

grpc::Status EchoServiceImpl::Echo(grpc::ServerContext* context,
                       const EchoRequest* request,
                       EchoResponse* response) {
  response->set_response(request->message());
  return grpc::Status::OK;
}