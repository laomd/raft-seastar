#pragma once

#include "echo.pb.h"

class EchoServiceImpl : public EchoService {
 public:
  virtual void Echo(::google::protobuf::RpcController* controller,
                       const EchoRequest* request,
                       EchoResponse* response,
                       ::google::protobuf::Closure* done) override;
};