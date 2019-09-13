#pragma once

#include <grpc++/grpc++.h>
#include "echo.grpc.pb.h"

class EchoServiceImpl : public EchoService::Service {
 public:
  virtual grpc::Status Echo(grpc::ServerContext* context,
                       const EchoRequest* request,
                       EchoResponse* response) override;
};