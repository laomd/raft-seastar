#pragma once

#include "serializer.hh"
#include <fmt/printf.h>

namespace laomd {
class rpc_protocol : public rpc::protocol<rpc_serializer> {
public:
  rpc_protocol() : rpc::protocol<rpc_serializer>(rpc_serializer()) {
    set_logger([](const seastar::sstring& log) { fmt::print("{}\n", log); });
  }
};

} // namespace laomd