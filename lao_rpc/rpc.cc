#include "rpc.hh"

BEGIN_NAMESPACE(laomd)

void RpcController::Reset() { };
bool RpcController::Failed() const { return false; };
std::string RpcController::ErrorText() const { return ""; };
void RpcController::StartCancel() { };
void RpcController::SetFailed(const std::string& /* reason */) { };
bool RpcController::IsCanceled() const { return false; };
void RpcController::NotifyOnCancel(::google::protobuf::Closure* /* callback */) { };

void RpcChannel::init(const std::string& ip, const int port) {
    _io = std::make_shared<boost::asio::io_service>();
    _sock = std::make_shared<boost::asio::ip::tcp::socket>(*_io);
    boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::address::from_string(ip), port);
    _sock->connect(ep);
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* /* controller */,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure*) {
    std::string serialzied_data = request->SerializeAsString();

    RpcMeta rpc_meta;
    rpc_meta.set_service_name(method->service()->name());
    rpc_meta.set_method_name(method->name());
    rpc_meta.set_data_size(serialzied_data.size());

    std::string serialzied_str = rpc_meta.SerializeAsString();

    int serialzied_size = serialzied_str.size();
    serialzied_str.insert(0, std::string((const char*)&serialzied_size, sizeof(int)));
    serialzied_str += serialzied_data;

    _sock->send(boost::asio::buffer(serialzied_str));

    char resp_data_size[sizeof(int)];
    _sock->receive(boost::asio::buffer(resp_data_size));

    int resp_data_len = *(int*)resp_data_size;
    std::vector<char> resp_data(resp_data_len, 0);
    _sock->receive(boost::asio::buffer(resp_data));

    response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
}

void RpcServer::add(::google::protobuf::Service* service) {
    ServiceInfo service_info;
    service_info.service = service;
    service_info.sd = service->GetDescriptor();
    for (int i = 0; i < service_info.sd->method_count(); ++i) {
        service_info.mds[service_info.sd->method(i)->name()] = service_info.sd->method(i);
    }

    _services[service_info.sd->name()] = service_info;
}

void RpcServer::start(const std::string& ip, const int port) {
    boost::asio::io_service io;
    boost::asio::ip::tcp::acceptor acceptor(
            io,
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(ip),
                port));

    while (true) {
        auto sock = std::make_shared<boost::asio::ip::tcp::socket>(io);
        acceptor.accept(*sock);

        LOG(INFO) << "recv from client: " << sock->remote_endpoint().address();

        //接收meta长度
        char meta_size[sizeof(int)];
        sock->receive(boost::asio::buffer(meta_size));

        int meta_len = *(int*)(meta_size);

        //接收meta数据
        std::vector<char> meta_data(meta_len, 0);
        sock->receive(boost::asio::buffer(meta_data));

        RpcMeta meta;
        meta.ParseFromString(std::string(&meta_data[0], meta_data.size()));

        //接收数据本身
        std::vector<char> data(meta.data_size(), 0);
        sock->receive(boost::asio::buffer(data));

        //
        dispatch_msg(
                meta.service_name(),
                meta.method_name(),
                std::string(&data[0], data.size()),
                sock);
    }
}

void RpcServer::dispatch_msg(
        const std::string& service_name,
        const std::string& method_name,
        const std::string& serialzied_data,
        const std::shared_ptr<boost::asio::ip::tcp::socket>& sock) {
    auto service = _services[service_name].service;
    auto md = _services[service_name].mds[method_name];

    LOG(INFO) << "recv service_name: " << service_name;
    LOG(INFO) << "recv method_name: " << method_name;
    LOG(INFO) << "recv type: " << md->input_type()->name();
    LOG(INFO) << "resp type: " << md->output_type()->name();

    auto recv_msg = service->GetRequestPrototype(md).New();
    recv_msg->ParseFromString(serialzied_data);
    auto resp_msg = service->GetResponsePrototype(md).New();

    RpcController controller;
    DoneParams params = {recv_msg, resp_msg, sock};
    auto done = ::google::protobuf::NewCallback(
            this,
            &RpcServer::on_resp_msg_filled,
            params);
    service->CallMethod(md, &controller, recv_msg, resp_msg, done);
}

void RpcServer::on_resp_msg_filled(DoneParams params) {
    auto [recv_msg, resp_msg, sock] = params;
    boost::scoped_ptr<::google::protobuf::Message> recv_msg_guard(recv_msg);
    boost::scoped_ptr<::google::protobuf::Message> resp_msg_guard(resp_msg);

    std::string resp_str;
    pack_message(resp_msg, &resp_str);

    sock->send(boost::asio::buffer(resp_str));
}

void RpcServer::pack_message(const ::google::protobuf::Message* msg, std::string* serialized_data) {
    int serialized_size = msg->ByteSize();
    serialized_data->assign(
                (const char*)&serialized_size,
                sizeof(serialized_size));
    msg->AppendToString(serialized_data);
}

END_NAMESPACE(laomd)