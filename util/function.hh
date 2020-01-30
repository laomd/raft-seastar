#pragma once

#include <exception>
#include <functional>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>

namespace laomd {
using steady_clock_type = std::chrono::steady_clock;

template <typename Exp> seastar::future<> ignore_exception(Exp &) {
  return seastar::make_ready_future();
}

seastar::future<> do_nothing() { return seastar::make_ready_future(); }

template <typename Clock = steady_clock_type, typename... T>
seastar::future<T...>
with_timeout(typename Clock::duration duration, seastar::future<T...> f,
             std::function<seastar::future<>(seastar::timed_out_error &)> &&
                 timedout_handler = ignore_exception<seastar::timed_out_error>,
             std::function<seastar::future<>()> &&handler = do_nothing) {
  return seastar::with_timeout(Clock::now() + duration, std::move(f))
      .then(handler)
      .handle_exception_type(timedout_handler);
}

template <typename... T>
seastar::future<T...> ignore_rpc_exceptions(seastar::future<T...> f) {
  return f.handle_exception_type(ignore_exception<seastar::rpc::closed_error>)
      .handle_exception_type(ignore_exception<seastar::rpc::timeout_error>)
      .handle_exception_type(ignore_exception<std::system_error>);
}

} // namespace laomd