#pragma once

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <string_view>
#include <format>

#if defined(SIMPERF_LIB)
	#include <spdlog/spdlog.h>
	#include <spdlog/common.h>
	#include <spdlog/fmt/ostr.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/sinks/stdout_color_sinks.h>
#else
	#include "../external/spdlog/include/spdlog/spdlog.h"
	#include "../external/spdlog/include/spdlog/common.h"
	#include "../external/spdlog/include/spdlog/fmt/ostr.h"
	#include "../external/spdlog/include/spdlog/sinks/basic_file_sink.h"
	#include "../external/spdlog/include/spdlog/sinks/stdout_color_sinks.h"
#endif

namespace simperf
{
#if defined(_WIN64)
	#define SIMPERF_DEBUGBREAK() __debugbreak()
#elif defined(__linux__)
	#include <signal.h>
	#define SIMPERF_DEBUGBREAK() raise(SIGTRAP)
#else
	#include <stdlib.h>
	#define SIMPERF_DEBUGBREAK() abort()
#endif

#if (defined(_DEBUG) && !defined(SIMPERF_DISABLE)) || defined(SIMPERF_ENABLE)
	#define SIMPERF_EXPAND_MACRO(x) x
	#define SIMPERF_STRINGIFY_MACRO(x) #x

	//need to attach with default error logger
	#define SIMPERF_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { SIMPERF_LOG_ASSERT(msg, __VA_ARGS__); SIMPERF_DEBUGBREAK(); } }
	#define SIMPERF_INTERNAL_ASSERT_WITH_MSG(type, check, ...) SIMPERF_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define SIMPERF_INTERNAL_ASSERT_NO_MSG(type, check) SIMPERF_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", SIMPERF_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define SIMPERF_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define SIMPERF_INTERNAL_ASSERT_GET_MACRO(...) SIMPERF_EXPAND_MACRO( SIMPERF_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, SIMPERF_INTERNAL_ASSERT_WITH_MSG, SIMPERF_INTERNAL_ASSERT_NO_MSG) )

	#define SIMPERF_ASSERT(...) SIMPERF_EXPAND_MACRO(SIMPERF_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#else
	#define SIMPERF_ASSERT(...) 
#endif

	typedef std::shared_ptr<spdlog::logger> logger;

	typedef std::vector<std::shared_ptr<spdlog::logger>> log_list;

	enum class register_result {
		failed = 0,
		ok,
	};

	inline bool operator!(register_result rr) {
		return rr == static_cast<register_result>(0);
	}

	inline std::ostream& operator<<(std::ostream& os, const register_result& rr) {
		os << ((!rr) ? "register_result::failed" : "register_result::ok");
		return os;
	}

	using namespace std::string_literals;
	using namespace std::string_view_literals;
	constexpr inline std::string_view TestFormat = "Test {} {}."sv;

	template<typename... Args>
	std::string Format(const std::string_view message, Args... formatItems)
	{
		return std::vformat(message, std::make_format_args(std::forward<Args>(formatItems)...));
	}

	class Log {
	public:
		template<typename ... Args>
		static void TestLog(const std::string& logger_name, const std::string_view fmt, Args&&... args) {
			::simperf::Log::GetLogger(logger_name)->trace(simperf::Format(fmt, std::forward<Args>(args)...));
		}
		static register_result RegisterStatic(const log_list& log_list_) {
			if (!s_StaticRegistryInitialized.load(std::memory_order_acquire)) {
				std::unique_lock<std::mutex> guard(s_RegisterLock);
				if (!s_StaticRegistryInitialized.load(std::memory_order_acquire)) {
					for (const auto& logger : log_list_) {
						auto result = RegisterLogger(logger);
						if (!result) {
							return register_result::failed;
						}
						else {
							if (s_DebugLogger.empty()) {
								s_DebugLogger = logger->name();
							}
							if (s_ErrorLogger.empty()) {
								s_ErrorLogger = logger->name();
							}
							s_StaticLoggerRegistry.insert({ logger->name(), logger });
						}
					}
				}
			}
			return register_result::ok;
		}

		static register_result RegisterDynamic(const logger& logger_) {
			auto result = RegisterLogger(logger_);
			if (!result) {
				return register_result::failed;
			}
			return register_result::ok;
		}

		static void Unregister(const std::string& name) {
			if (s_StaticLoggerRegistry.erase(name)) {
				spdlog::drop(name);
			}
			else {
				spdlog::drop(name);
			}
		}

		static logger GetLogger(const std::string& logger_name) {
			auto found = s_StaticLoggerRegistry.find(logger_name);
			return found == s_StaticLoggerRegistry.end() ? spdlog::get(logger_name) : found->second;
		}

		static void SetDebugLogger(const std::string& logger_name) {
			auto found = s_StaticLoggerRegistry.find(logger_name);
			if (found != s_StaticLoggerRegistry.end()) {
				s_DebugLogger = logger_name;
			}
		}

		static void SetErrorLogger(const std::string& logger_name) {
			auto found = s_StaticLoggerRegistry.find(logger_name);
			if (found != s_StaticLoggerRegistry.end()) {
				s_ErrorLogger = logger_name;
			}
		}

		static const std::string& GetDebugLoggerName(void) {
			return s_DebugLogger;
		}

		static const std::string& GetErrorLoggerName(void) {
			return s_ErrorLogger;
		}

	private:
		static register_result RegisterLogger(const logger& logger_) {
			try {
				spdlog::register_logger(logger_);
				return register_result::ok;
			}
			catch (spdlog::spdlog_ex& e) {
				std::cerr << e.what() << std::endl;
				return register_result::failed;
			}
		}

	private:
		inline static std::string s_DebugLogger;
		inline static std::string s_ErrorLogger;
		inline static std::mutex s_RegisterLock;
		inline static std::atomic_bool s_StaticRegistryInitialized;
		inline static std::unordered_map<std::string, logger> s_StaticLoggerRegistry;
	};

#define SIMPERF_LOG_IT_(logger_name, log_level, ...)  \
	{ \
		simperf::logger logger_ = simperf::Log::GetLogger(logger_name); \
		logger_ == nullptr ? ((void)0) : \
		log_level == ::spdlog::level::trace ? logger_->trace(__VA_ARGS__) : \
		log_level == ::spdlog::level::debug ? logger_->debug(__VA_ARGS__) : \
		log_level == ::spdlog::level::info ? logger_->info(__VA_ARGS__) : \
		log_level == ::spdlog::level::warn ? logger_->warn(__VA_ARGS__) : \
		log_level == ::spdlog::level::err ? logger_->error(__VA_ARGS__) : \
		log_level == ::spdlog::level::critical ? logger_->critical(__VA_ARGS__) : \
		((void)0); \
	}

#define SIMPERF_LOG_IT__(logger_name, ...) ::simperf::Log::TestLog(logger_name, __VA_ARGS__);

#if (defined(_DEBUG) && !defined(SIMPERF_DISABLE)) || defined(SIMPERF_ENABLE)
	#define SIMPERF_TRACE(logger_name, ...)     SIMPERF_LOG_IT_(logger_name, spdlog::level::trace, __VA_ARGS__)
	#define SIMPERF_DEBUG(logger_name, ...)     SIMPERF_LOG_IT_(logger_name, spdlog::level::debug, __VA_ARGS__)
	#define SIMPERF_INFO(logger_name, ...)      SIMPERF_LOG_IT_(logger_name, spdlog::level::info, __VA_ARGS__)
	#define SIMPERF_WARN(logger_name, ...)      SIMPERF_LOG_IT_(logger_name, spdlog::level::warn, __VA_ARGS__)
	#define SIMPERF_ERROR(logger_name, ...)     SIMPERF_LOG_IT_(logger_name, spdlog::level::err, __VA_ARGS__)
	#define SIMPERF_CRITICAL(logger_name, ...)  SIMPERF_LOG_IT_(logger_name, spdlog::level::critical, __VA_ARGS__)

	#define SIMPERF_LOG_PROFILE(...)			SIMPERF_DEBUG(::simperf::Log::GetDebugLoggerName(), __VA_ARGS__)
	#define SIMPERF_LOG_ASSERT(...)             SIMPERF_ERROR(::simperf::Log::GetErrorLoggerName(), __VA_ARGS__)
#else
	#define SIMPERF_TRACE(logger_name, ...)   
	#define SIMPERF_DEBUG(logger_name, ...)   
	#define SIMPERF_INFO(logger_name, ...)    
	#define SIMPERF_WARN(logger_name, ...)    
	#define SIMPERF_ERROR(logger_name, ...)   
	#define SIMPERF_CRITICAL(logger_name, ...)
	#define SIMPERF_LOG_ASSERT(...)

	#define SIMPERF_LOG_PROFILE(...)
	#define SIMPERF_LOG_ASSERT(...) 
#endif

	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;

		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	class Instrumentor
	{
	public:
		Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead.  That's better than having badly formatted
				// profiling output.
				//if (mcctt::core::Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
				//{
				//	MCCTT_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
				//}
				InternalEndSession();
			}
			m_OutputStream.open(filepath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({ name });
				WriteHeader();
			}
			else
			{
				//if (mcctt::core::Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
				//{
				//	MCCTT_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
				//}
			}
		}

		void EndSession()
		{
			std::lock_guard lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}
	private:
		Instrumentor()
			: m_CurrentSession(nullptr)
		{
		}

		~Instrumentor()
		{
			EndSession();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
			m_OutputStream.flush();
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		// Note: you must already own lock on m_Mutex before
		// calling InternalEndSession()
		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				WriteFooter();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}
	private:
		std::mutex m_Mutex;
		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char* name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			SIMPERF_LOG_PROFILE("{0} on thread {1} took {2}", m_Name, std::this_thread::get_id(), elapsedTime);
			Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });
			m_Stopped = true;
		}
	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
	};

	namespace InstrumentorUtils {

		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
#if (defined(_DEBUG) && !defined(SIMPERF_DISABLE)) || defined(SIMPERF_ENABLE)
	// Resolve which function signature macro will be used. Note that this only
	// is resolved when the (pre)compiler starts, so the syntax highlighting
	// could mark the wrong one in your editor!
	#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
		#define SIMPERF_FUNC_SIG __PRETTY_FUNCTION__
	#elif defined(__DMC__) && (__DMC__ >= 0x810)
		#define SIMPERF_FUNC_SIG __PRETTY_FUNCTION__
	#elif (defined(__FUNCSIG__) || (_MSC_VER))
		#define SIMPERF_FUNC_SIG __FUNCSIG__
	#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
		#define SIMPERF_FUNC_SIG __FUNCTION__
	#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
		#define SIMPERF_FUNC_SIG __FUNC__
	#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
		#define SIMPERF_FUNC_SIG __func__
	#elif defined(__cplusplus) && (__cplusplus >= 201103)
		#define SIMPERF_FUNC_SIG __func__
	#else
		#define SIMPERF_FUNC_SIG "SIMPERF_FUNC_SIG unknown!"
	#endif

	#define SIMPERF_PROFILE_BEGIN_SESSION(name, filepath) ::simperf::Instrumentor::Get().BeginSession(name, filepath)
	#define SIMPERF_PROFILE_END_SESSION() ::simperf::Instrumentor::Get().EndSession()
	#define SIMPERF_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::simperf::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
												   ::simperf::InstrumentationTimer timer##line(fixedName##line.Data)
	#define SIMPERF_PROFILE_SCOPE_LINE(name, line) SIMPERF_PROFILE_SCOPE_LINE2(name, line)
	#define SIMPERF_PROFILE_SCOPE(name) SIMPERF_PROFILE_SCOPE_LINE(name, __LINE__)
	#define SIMPERF_PROFILE_FUNCTION() SIMPERF_PROFILE_SCOPE(SIMPERF_FUNC_SIG)
#else
	#define SIMPERF_PROFILE_BEGIN_SESSION(name, filepath)
	#define SIMPERF_PROFILE_END_SESSION()
	#define SIMPERF_PROFILE_SCOPE(name)
	#define SIMPERF_PROFILE_FUNCTION()
#endif
}