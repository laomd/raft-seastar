#pragma once

#include <functional>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>

namespace laomd {
using steady_clock_type = std::chrono::steady_clock;

seastar::future<> do_nothing() { return seastar::make_ready_future(); }

template <typename Exp, typename... T>
seastar::future<> ignore_exception(seastar::future<T...> f) {
  return f.handle_exception([](auto &&exptr) {
    try {
      std::rethrow_exception(exptr);
    } catch (Exp &e) {
      return seastar::make_ready_future();
    }
  });
}

template <typename Clock = steady_clock_type, typename... T>
seastar::future<> with_timeout(
    typename Clock::duration duration, seastar::future<T...> f) {
  return ignore_exception<seastar::timed_out_error>(seastar::with_timeout(Clock::now() + duration, std::move(f)));
}
} // namespace laomd