#pragma once

#include <seastar/core/future.hh>

namespace laomd {
  seastar::future<> do_nothing() {
    return seastar::make_ready_future();
  }
  
  seastar::future<> ignore_exception(std::exception_ptr exp) {
    return seastar::make_ready_future();
  }
}