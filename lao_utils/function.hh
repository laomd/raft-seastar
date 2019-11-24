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

template <typename Clock = steady_clock_type, typename... T>
seastar::future<T...> with_timeout(
    typename Clock::duration duration, seastar::future<T...> f,
    std::function<seastar::future<>(seastar::timed_out_error &)> &&handler =
        ignore_exception<seastar::timed_out_error>) {
  return seastar::with_timeout(Clock::now() + duration, std::move(f))
      .handle_exception_type(handler);
}
} // namespace laomd