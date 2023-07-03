#pragma once

#define SIMPERF_LOG_IT(logger_name, log_level, ...) ::simperf::ctx::LogIt(logger_name, log_level, __VA_ARGS__)
#define SIMPERF_FORMATTED_LOG_IT(logger_name, log_level, ...) ::simperf::ctx::FormattedLogIt(logger_name, log_level, __VA_ARGS__)
#define SIMPERF_AUTOFORMATTED_LOG_IT(logger_name, log_level, ...) ::simperf::ctx::AutoFormattedLogIt(logger_name, log_level, __VA_ARGS__)

#if (defined(_DEBUG) && !defined(SIMPERF_DISABLE)) || defined(SIMPERF_ENABLE)
	#define SIMPERF_TRACE(logger_name, ...)     SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::trace,    __VA_ARGS__)
	#define SIMPERF_DEBUG(logger_name, ...)     SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::debug,    __VA_ARGS__)
	#define SIMPERF_INFO(logger_name, ...)      SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::info,     __VA_ARGS__)
	#define SIMPERF_WARN(logger_name, ...)      SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::warn,     __VA_ARGS__)
	#define SIMPERF_ERROR(logger_name, ...)     SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::err,      __VA_ARGS__)
	#define SIMPERF_CRITICAL(logger_name, ...)  SIMPERF_FORMATTED_LOG_IT(logger_name, spdlog::level::critical, __VA_ARGS__)

	#if defined(_WIN64)
		#define SIMPERF_DEBUGBREAK() __debugbreak()
	#elif defined(__linux__)
		#include <signal.h>
		#define SIMPERF_DEBUGBREAK() raise(SIGTRAP)
	#else
		#include <stdlib.h>
		#define SIMPERF_DEBUGBREAK() abort()
	#endif

	#define SIMPERF_RESOLVE_LOG_LEVEL(logger, source) \
		::simperf::s_LoggerTargets.at(logger).second.at(source)

	#define SIMPERF_LOG_ASSERT(logger, msg, ...) \
		SIMPERF_AUTOFORMATTED_LOG_IT(logger, SIMPERF_RESOLVE_LOG_LEVEL(logger, ::simperf::AssertionLogSource::Throw), msg, __VA_ARGS__)

	#define SIMPERF_ASSERT(check, logger, msg, ...)  \
		if(!(check)) { \
			SIMPERF_LOG_ASSERT(logger, msg, __VA_ARGS__); \
			SIMPERF_DEBUGBREAK(); \
		}
	#define SIMPERF_ASSERT_EQ(left, right, logger, msg, ...) { SIMPERF_ASSERT(left == right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_NEQ(left, right, logger, msg, ...) { SIMPERF_ASSERT(left != right, logger, msg, __VA_ARGS__) }

#else
	#define SIMPERF_TRACE(logger_name, ...) 
	#define SIMPERF_DEBUG(logger_name, ...)   
	#define SIMPERF_INFO(logger_name, ...)    
	#define SIMPERF_WARN(logger_name, ...)    
	#define SIMPERF_ERROR(logger_name, ...)   
	#define SIMPERF_CRITICAL(logger_name, ...)

	#define SIMPERF_DEBUGBREAK()
#endif