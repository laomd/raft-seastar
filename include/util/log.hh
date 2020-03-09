// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

#include "util/macros.hh"
#include <fmt/printf.h>
#include <seastar/util/log.hh>

namespace laomd {

#define LOG_DECLARE() static seastar::logger logger_
#define LOG_SETUP(cls) seastar::logger cls::logger_(#cls)

inline seastar::logger& default_logger() {
  static seastar::logger logger("root");
  return logger;
}

namespace {
/// \brief compile time log helper to print log file name.
/// @sz must be inclusive
static constexpr const char *find_last_slash(const char *file, std::size_t sz,
                                             char x) {
  return sz == 0
             ? file
             : file[sz] == x ? &file[sz + 1] : find_last_slash(file, sz - 1, x);
}

template <typename... Args> inline void noop(Args &&... args) { ((void)0); }
} // namespace

#define __FILENAME__ find_last_slash(__FILE__, ARRAYSIZE(__FILE__) - 1, '/')
#define LOG_INFO(format, args...)                                              \
  logger_.info("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_ERROR(format, args...)                                             \
  logger_.error("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_WARN(format, args...)                                              \
  logger_.warn("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_DEBUG(format, args...)                                             \
  logger_.debug("{}:{}] " format, __FILENAME__, __LINE__, ##args)
#define LOG_TRACE(format, args...)                                             \
  do {                                                                         \
    if (UNLIKELY(logger_.is_enabled(seastar::log_level::trace))) {             \
      logger_.trace("{}:{}] " format, __FILENAME__, __LINE__, ##args);         \
    }                                                                          \
  } while (false)

#define LOG_THROW(format, args...)                                             \
  do {                                                                         \
    fmt::memory_buffer __smflog_w;                                             \
    fmt::format_to(__smflog_w, "{}:{}] " format, __FILENAME__, __LINE__,       \
                   ##args);                                                    \
    logger_.error(__smflog_w.data());                                          \
    throw std::runtime_error(__smflog_w.data());                               \
  } while (false)

#define LOG_ERROR_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.error("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__, \
                    ##args);                                                   \
    }                                                                          \
  } while (false)
#define LOG_DEBUG_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.debug("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__, \
                    ##args);                                                   \
    }                                                                          \
  } while (false)
#define LOG_WARN_IF(condition, format, args...)                                \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.warn("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__,  \
                   ##args);                                                    \
    }                                                                          \
  } while (false)
#define LOG_TRACE_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.trace("{}:{}] (" #condition ") " format, __FILENAME__, __LINE__, \
                    ##args);                                                   \
    }                                                                          \
  } while (false)
#define LOG_THROW_IF(condition, format, args...)                               \
  do {                                                                         \
    if (UNLIKELY(condition)) {                                                 \
      fmt::memory_buffer __smflog_w;                                           \
      fmt::format_to(__smflog_w, "{}:{}] (" #condition ") " format,            \
                     __FILENAME__, __LINE__, ##args);                          \
      logger_.error(__smflog_w.data());                                        \
      throw std::runtime_error(__smflog_w.data());                             \
    }                                                                          \
  } while (false)

#ifndef NDEBUG

#define DLOG_THROW(format, args...)                                            \
  do {                                                                         \
    fmt::memory_buffer __smflog_w;                                             \
    fmt::format_to(__smflog_w, "D {}:{}] " format, __FILENAME__, __LINE__,     \
                   ##args);                                                    \
    logger_.error(__smflog_w.data());                                          \
    throw std::runtime_error(__smflog_w.data());                               \
  } while (false)

#define DLOG_INFO(format, args...)                                             \
  logger_.info("D {}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_ERROR(format, args...)                                            \
  logger_.error("D {}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_WARN(format, args...)                                             \
  logger_.warn("D {}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_DEBUG(format, args...)                                            \
  logger_.debug("D {}:{}] " format, __FILENAME__, __LINE__, ##args)
#define DLOG_TRACE(format, args...)                                            \
  do {                                                                         \
    if (UNLIKELY(logger_.is_enabled(seastar::log_level::trace))) {             \
      logger_.trace("D {}:{}] " format, __FILENAME__, __LINE__, ##args);       \
    }                                                                          \
  } while (false)

#define DLOG_INFO_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.info("D {}:{}] (" #condition ") " format, __FILENAME__,          \
                   __LINE__, ##args);                                          \
    }                                                                          \
  } while (false)
#define DLOG_ERROR_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.error("D {}:{}] (" #condition ") " format, __FILENAME__,         \
                    __LINE__, ##args);                                         \
    }                                                                          \
  } while (false)
#define DLOG_DEBUG_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.debug("D {}:{}] (" #condition ") " format, __FILENAME__,         \
                    __LINE__, ##args);                                         \
    }                                                                          \
  } while (false)

#define DLOG_WARN_IF(condition, format, args...)                               \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.warn("D {}:{}] (" #condition ") " format, __FILENAME__,          \
                   __LINE__, ##args);                                          \
    }                                                                          \
  } while (false)
#define DLOG_TRACE_IF(condition, format, args...)                              \
  do {                                                                         \
    if (condition) {                                                           \
      logger_.trace("D {}:{}] (" #condition ") " format, __FILENAME__,         \
                    __LINE__, ##args);                                         \
    }                                                                          \
  } while (false)
#define DLOG_THROW_IF(condition, format, args...)                              \
  do {                                                                         \
    if (UNLIKELY(condition)) {                                                 \
      fmt::memory_buffer __smflog_w;                                           \
      fmt::format_to(__smflog_w, "D {}:{}] (" #condition ") " format,          \
                     __FILENAME__, __LINE__, ##args);                          \
      logger_.error(__smflog_w.data());                                        \
      throw std::runtime_error(__smflog_w.data());                             \
    }                                                                          \
  } while (false)

#else
#define DTHROW_IFNULL(x) (x)
#define DLOG_INFO(format, args...) ((void)0)
#define DLOG_ERROR(format, args...) ((void)0)
#define DLOG_WARN(format, args...) ((void)0)
#define DLOG_DEBUG(format, args...) ((void)0)
#define DLOG_TRACE(format, args...) ((void)0)
#define DLOG_THROW(format, args...) ((void)0)
#define DLOG_INFO_IF(condition, format, args...) ((void)0)
#define DLOG_ERROR_IF(condition, format, args...) ((void)0)
#define DLOG_DEBUG_IF(condition, format, args...) ((void)0)
#define DLOG_WARN_IF(condition, format, args...) ((void)0)
#define DLOG_TRACE_IF(condition, format, args...) ((void)0)
#define DLOG_THROW_IF(condition, format, args...)                              \
  noop(condition, format, ##args);
#endif

} // namespace laomd
