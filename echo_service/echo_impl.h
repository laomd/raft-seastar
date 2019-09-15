#pragma once

#include "echo.grpc.pb.h"
#include <grpc++/grpc++.h>

class EchoServiceImpl : public EchoService::Service {
public:
  virtual grpc::Status Echo(grpc::ServerContext *context,
                            const EchoRequest *request,
                            EchoResponse *response) override;
};