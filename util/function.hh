#pragma once

#include <exception>
#include <functional>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>

namespace laomd {

template <typename Exp> seastar::future<> ignore_exception(Exp &) {
  return seastar::make_ready_future();
}

template <typename Clock, typename... T>
seastar::future<T...> with_timeout(typename Clock::duration duration,
                                   seastar::future<T...> f) {
  return seastar::with_timeout(Clock::now() + duration, std::move(f));
}

seastar::future<> ignore_rpc_exceptions(seastar::future<> f) {
  return f.handle_exception_type(ignore_exception<seastar::rpc::closed_error>)
      .handle_exception_type(ignore_exception<seastar::rpc::timeout_error>)
      .handle_exception_type(ignore_exception<std::system_error>);
}

} // namespace laomd