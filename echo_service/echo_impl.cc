#include "echo_impl.h"

void EchoServiceImpl::Echo(::google::protobuf::RpcController* controller,
                  const EchoRequest* request,
                  EchoResponse* response,
                  ::google::protobuf::Closure* done) {
  response->set_response(request->message());
  if (done) {
    done->Run();
  }
}