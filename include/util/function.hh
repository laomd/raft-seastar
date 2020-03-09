#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <random>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/sleep.hh>

namespace laomd {

template <typename Clock, typename... T>
seastar::future<T...> with_timeout(typename Clock::duration duration,
                                   seastar::future<T...> f) {
  return seastar::with_timeout(Clock::now() + duration, std::move(f));
}

} // namespace laomd