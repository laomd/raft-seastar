#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>

#include <lao_utils/common.hh>
#include <lao_utils/logger.hh>
#include <lao_rpc/rpc_meta.pb.h>

BEGIN_NAMESPACE(laomd)

class RpcController : public ::google::protobuf::RpcController {
public:
  virtual void Reset() override;

  virtual bool Failed() const override;
  virtual std::string ErrorText() const override;
  virtual void StartCancel() override;
  virtual void SetFailed(const std::string& /* reason */) override;
  virtual bool IsCanceled() const override;
  virtual void NotifyOnCancel(::google::protobuf::Closure* /* callback */) override;
};

class RpcChannel : public ::google::protobuf::RpcChannel {
public:
    void init(const std::string& ip, const int port);

    virtual void CallMethod(
            const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* /* controller */,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure*) override;

private:
    std::shared_ptr<boost::asio::io_service> _io;
    std::shared_ptr<boost::asio::ip::tcp::socket> _sock;
};

class RpcServer {
public:
    void add(::google::protobuf::Service* service);
    void start(const std::string& ip, const int port);
private:
    struct DoneParams {
        ::google::protobuf::Message* recv_msg;
        ::google::protobuf::Message* resp_msg;
        std::shared_ptr<boost::asio::ip::tcp::socket> sock;
    };

    struct ServiceInfo {
        ::google::protobuf::Service* service;
        const ::google::protobuf::ServiceDescriptor* sd;
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> mds;
    };

    void dispatch_msg(
            const std::string& service_name,
            const std::string& method_name,
            const std::string& serialzied_data,
            const std::shared_ptr<boost::asio::ip::tcp::socket>& sock);
    void on_resp_msg_filled(DoneParams params);
    void pack_message(const ::google::protobuf::Message* msg, std::string* serialized_data);

    //service_name -> {Service*, ServiceDescriptor*, MethodDescriptor* []}
    std::map<std::string, ServiceInfo> _services;
};

END_NAMESPACE(laomd)