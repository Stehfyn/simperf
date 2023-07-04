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
		::simperf::s_LogLevelTargets.at(logger).second.at(source)

	#define SIMPERF_LOG_FLUSH(logger) \
		::spdlog::get(logger)->flush()

	#define SIMPERF_LOG_ASSERT(logger, msg, ...) \
		SIMPERF_FORMATTED_LOG_IT(logger, SIMPERF_RESOLVE_LOG_LEVEL(logger, ::simperf::AssertionLogSource::Throw), msg, __VA_ARGS__); \
		SIMPERF_LOG_FLUSH(logger)

	#define SIMPERF_LOG_ASSERT_NO_THROW(logger, msg, ...) \
		SIMPERF_FORMATTED_LOG_IT(logger, SIMPERF_RESOLVE_LOG_LEVEL(logger, ::simperf::AssertionLogSource::ExplicitNoThrow), msg, __VA_ARGS__); \
		SIMPERF_LOG_FLUSH(logger)

	#define SIMPERF_LOG_ASSERT_VARIABLE_THROW(logger, msg, ...) \
		SIMPERF_FORMATTED_LOG_IT(logger, SIMPERF_RESOLVE_LOG_LEVEL(logger, ::simperf::AssertionLogSource::VariableNoThrow), msg, __VA_ARGS__); \
		SIMPERF_LOG_FLUSH(logger)

	#define SIMPERF_ASSERT(check, logger, msg, ...)  \
		if(!(check)) { \
			SIMPERF_LOG_ASSERT(logger, msg, __VA_ARGS__); \
			SIMPERF_DEBUGBREAK(); \
		}

	#define SIMPERF_ASSERT_NO_THROW(check, logger, msg, ...) \
		if(!(check)) { \
			SIMPERF_LOG_ASSERT_NO_THROW(logger, msg, __VA_ARGS__); \
		}
	
	#define SIMPERF_ASSERT_VARIABLE_THROW(check, logger, msg, ...)  \
		if(!(check)) { \
			if (::simperf::ctx::VariableShouldThrow()) { \
				SIMPERF_LOG_ASSERT(logger, msg, __VA_ARGS__); \
				SIMPERF_DEBUGBREAK(); \
			} \
			else { \
				SIMPERF_LOG_ASSERT_VARIABLE_THROW(logger, msg, __VA_ARGS__); \
			} \
		}

	#define SIMPERF_ASSERT_EQ(left, right, logger, msg, ...) { SIMPERF_ASSERT(left == right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_NEQ(left, right, logger, msg, ...) { SIMPERF_ASSERT(left != right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_GT(left, right, logger, msg, ...) { SIMPERF_ASSERT(left < right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_GTE(left, right, logger, msg, ...) { SIMPERF_ASSERT(left <= right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_LT(left, right, logger, msg, ...) { SIMPERF_ASSERT(left <= right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_LTE(left, right, logger, msg, ...) { SIMPERF_ASSERT(left <= right, logger, msg, __VA_ARGS__) }

	#define SIMPERF_ASSERT_EQ_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left == right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_NEQ_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left != right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_GT_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left < right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_GTE_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left <= right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_LT_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left <= right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_LTE_NO_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_NO_THROW(left <= right, logger, msg, __VA_ARGS__) }

	#define SIMPERF_ASSERT_EQ_VARIABLE_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_VARIABLE_THROW(left == right, logger, msg, __VA_ARGS__) }
	#define SIMPERF_ASSERT_NEQ_VARIABLE_THROW(left, right, logger, msg, ...) { SIMPERF_ASSERT_VARIABLE_THROW(left != right, logger, msg, __VA_ARGS__) }

#else
	#define SIMPERF_TRACE(logger_name, ...) 
	#define SIMPERF_DEBUG(logger_name, ...)   
	#define SIMPERF_INFO(logger_name, ...)    
	#define SIMPERF_WARN(logger_name, ...)    
	#define SIMPERF_ERROR(logger_name, ...)   
	#define SIMPERF_CRITICAL(logger_name, ...)

	#define SIMPERF_DEBUGBREAK()
#endif