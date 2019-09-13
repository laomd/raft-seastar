#include <iostream>
#include <grpc++/grpc++.h>
#include "echo.grpc.pb.h"

using namespace grpc;

int main() {
    auto channel = grpc::CreateChannel(
                "127.0.0.1:6688", grpc::InsecureChannelCredentials());

    EchoRequest request;
    EchoResponse response;
    request.set_message("hello, myrpc.");

    ClientContext context;
    auto stub(EchoService::NewStub(channel));
    stub->Echo(&context, request, &response);
    std::cout << "resp: " << response.response() << std::endl;

    return 0;
}