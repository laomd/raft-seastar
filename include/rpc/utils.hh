#pragma once

#include <chrono>
#include <random>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/sleep.hh>

namespace laomd {

template <typename Exp> seastar::future<> ignore_exception(Exp &) {
  return seastar::make_ready_future();
}

inline seastar::future<> ignore_rpc_exceptions(seastar::future<> f) {
  return f.handle_exception_type(ignore_exception<seastar::rpc::closed_error>)
      .handle_exception_type(ignore_exception<std::system_error>)
      .handle_exception_type(ignore_exception<seastar::rpc::timeout_error>);
}

namespace {
uint32_t pow_fast(uint32_t base, uint32_t power) {
  if (power == 0) {
    return 1;
  } else {
    uint32_t half = pow_fast(base, power / 2);
    return half * half * ((power & 1) ? base : 1);
  }
}
} // namespace

template <typename... T>
seastar::future<T...>
with_backoff(uint32_t try_count,
             std::function<seastar::future<T...>(uint32_t)> func) {
  return seastar::do_with(
      (uint32_t)0, seastar::promise<T...>(),
      [func = std::move(func), try_count](uint32_t &count, auto &pr) mutable {
        (void)seastar::repeat([func = std::move(func), &count, &pr,
                               try_count]() mutable {
          return func(count).then_wrapped([&count, &pr, try_count](auto fut) {
            if (fut.failed()) {
              count++;
              auto eptr = fut.get_exception();
              if (count >= try_count) {
                pr.set_exception(eptr);
                return seastar::stop_iteration::yes;
              }
              return seastar::stop_iteration::no;
            } else {
              pr.set_value(fut.get());
              return seastar::stop_iteration::yes;
            }
          });
        });
        return pr.get_future();
      });
}

} // namespace laomd