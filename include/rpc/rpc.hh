#pragma once

#include "serializer.hh"
#include "util/log.hh"
#include <seastar/net/api.hh>
#include <seastar/net/socket_defs.hh>

namespace laomd {
using seastar::server_socket;
using seastar::socket;
using seastar::socket_address;
using seastar::rpc::client_options;
using seastar::rpc::resource_limits;
using seastar::rpc::server_options;

class rpc_protocol : public rpc::protocol<rpc_serializer> {
  LOG_DECLARE();

public:
  rpc_protocol() : rpc::protocol<rpc_serializer>(rpc_serializer()) {
    set_logger([](const seastar::sstring &log) { DLOG_ERROR("{}", log); });
  }
};

struct rpc_service {
  virtual void start() = 0;
  virtual seastar::future<> stop() = 0;
  virtual uint64_t service_id() const = 0;
  virtual const char* name() const = 0;
  virtual void on_register(rpc_protocol &proto, uint64_t rpc_verb_base) = 0;
  virtual ~rpc_service() = default;
};

class rpc_server : public rpc_protocol::server {
  rpc_protocol proto;
  std::deque<std::unique_ptr<rpc_service>> services_;
  seastar::ipv4_addr addr_;
  LOG_DECLARE();

public:
  rpc_server(const socket_address &addr,
             resource_limits memory_limit = resource_limits())
      : proto(), rpc_protocol::server(proto, addr, memory_limit), addr_(addr) {}
  rpc_server(server_options opts, const socket_address &addr,
             resource_limits memory_limit = resource_limits())
      : proto(), rpc_protocol::server(proto, opts, addr, memory_limit),
        addr_(addr) {}
  rpc_server(server_socket socket,
             resource_limits memory_limit = resource_limits(),
             server_options opts = server_options{})
      : proto(), rpc_protocol::server(proto, std::move(socket), memory_limit),
        addr_(socket.local_address()) {}
  rpc_server(server_options opts, server_socket socket,
             resource_limits memory_limit = resource_limits())
      : proto(), rpc_protocol::server(proto, opts, std::move(socket),
                                      memory_limit),
        addr_(socket.local_address()) {}

  template <typename Service, typename... Args>
  Service *register_service(Args... args) {
    // at most 2^8 rpc handler for each rpc service
    auto service = std::make_unique<Service>(std::move(args)...);
    auto service_id = service->service_id() << 8;
    LOG_INFO("register rpc service {} with global service id {}", service->name(), service_id);
    service->on_register(proto, service_id);
    Service *origin_service = service.get();
    services_.emplace_back(std::move(service));
    return origin_service;
  }

  void start();
  seastar::future<> stop();
};

class rpc_client : public rpc_protocol::client {
  rpc_protocol proto;

public:
  rpc_client(const socket_address &addr, const socket_address &local = {})
      : rpc_protocol::client(proto, addr, local) {}
  rpc_client(client_options options, const socket_address &addr,
             const socket_address &local = {})
      : rpc_protocol::client(proto, options, addr, local) {}
  rpc_client(socket socket, const socket_address &addr,
             const socket_address &local = {})
      : rpc_protocol::client(proto, std::move(socket), addr, local) {}
  rpc_client(client_options options, socket socket, const socket_address &addr,
             const socket_address &local = {})
      : rpc_protocol::client(proto, options, std::move(socket), addr, local) {}

  void register_service(rpc_service &service) {
    // at most 2^8 rpc handlers for each rpc service
    service.on_register(proto, service.service_id() << 8);
  }

  template <typename Func> auto get_handler(rpc_service &service, uint32_t id) {
    uint64_t rpc_verb_base = service.service_id() << 8;
    return proto.make_client<Func>(rpc_verb_base + id);
  }
};

} // namespace laomd