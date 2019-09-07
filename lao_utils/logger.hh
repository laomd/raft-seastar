#pragma once

#include <seastar/util/log.hh>
#include "lao_utils/common.hh"

BEGIN_NAMESPACE(laomd)

#define LOGGER_DECL(name) \
	static seastar::logger logger_(#name)

#define LOG_INFO logger_.info
#define LOG_DEBUG logger_.debug
#define LOG_TRACE logger_.trace
#define LOG_WARN logger_.warning
#define LOG_ERROR logger_.error

END_NAMESPACE(laomd)
