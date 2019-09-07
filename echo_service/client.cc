#include <iostream>
#include "echo.pb.h"
#include "lao_rpc/rpc.hh"

using namespace laomd;

int main() {
    RpcChannel channel;
    channel.init("127.0.0.1", 6688);

    EchoRequest request;
    EchoResponse response;
    request.set_message("hello, myrpc.");

    EchoService_Stub stub(&channel);
    RpcController cntl;
    stub.Echo(&cntl, &request, &response, NULL);
    std::cout << "resp: " << response.response() << std::endl;

    return 0;
}