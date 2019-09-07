#pragma once
#include <boost/log/trivial.hpp>
#include "console_color.hh"

#define MY_LOG(level) BOOST_LOG_TRIVIAL(level) << YELLOW \
    << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__\
    << ORIGIN << " "

#define LOG_INFO MY_LOG(info)
#define LOG_DEBUG MY_LOG(debug)
#define LOG_TRACE MY_LOG(trace)
#define LOG_WARN MY_LOG(warning)
#define LOG_ERROR MY_LOG(error)
#define LOG_FATAL MY_LOG(fatal)