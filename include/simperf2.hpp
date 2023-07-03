#pragma once

#include <any>
#include <mutex>
#include <memory>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <atomic>
#include <unordered_map>
#include <string>
#include <utility>
#include <format>
#include <array>


#if defined(SIMPERF_LIB)
	//#include <spdlog/spdlog.h>
	//#include <spdlog/common.h>
	//#include <spdlog/fmt/ostr.h>
	//#include <spdlog/sinks/basic_file_sink.h>
	//#include <spdlog/sinks/stdout_color_sinks.h>
//#define SPDLOG_USE_STD_FORMAT
	#include <spdlog_setup/conf.h>
#else
	#include "../external/spdlog/include/spdlog/spdlog.h"
	#include "../external/spdlog/include/spdlog/common.h"
	#include "../external/spdlog/include/spdlog/fmt/ostr.h"
	#include "../external/spdlog/include/spdlog/sinks/basic_file_sink.h"
	#include "../external/spdlog/include/spdlog/sinks/stdout_color_sinks.h"
	#include "../external/spdlog_setup/include/spdlog_setup/conf.h"
#endif

#include "details/log-impl.h"
#include "details/assert-impl.h"

namespace simperf {
	enum class ProfilingLogSource {
		FunctionSignature,
		ScopeValueTracking,
		ScopeTiming,
	};

	enum class AssertionLogSource {
		ValueCheck,
		ExplicitNoThrow,
		VariableNoThrow,
		Throw
	};

	using sp_log_level = spdlog::level::level_enum;

	typedef std::unordered_map<ProfilingLogSource, sp_log_level> ProfilingLogTargets;
	typedef std::unordered_map<AssertionLogSource, sp_log_level> AssertionLogTargets;

	typedef std::pair<ProfilingLogTargets, AssertionLogTargets> DefaultLogTargets;

	DefaultLogTargets get_default_log_targets(void) {
		return {
			{{ProfilingLogSource::FunctionSignature, sp_log_level::trace},
			{ProfilingLogSource::ScopeValueTracking, sp_log_level::debug},
			{ProfilingLogSource::ScopeTiming,        sp_log_level::debug}},

			{{AssertionLogSource::ValueCheck,        sp_log_level::info},
			{AssertionLogSource::ExplicitNoThrow,    sp_log_level::warn},
			{AssertionLogSource::VariableNoThrow,    sp_log_level::err},
			{AssertionLogSource::Throw,              sp_log_level::critical}}
		};
	}

	std::unordered_map<std::string, DefaultLogTargets> s_LoggerTargets{
		{"simperf", get_default_log_targets() }
	};
}

namespace simperf 
{
	inline void default_initialize(void);
	inline void initialize_from_config(const char* path);
    inline void initialize_from_log_specs();

	class ctx {
	public:
		template <typename T>
		inline static void LogIt(const std::string& logger_name, 
			const sp_log_level& log_level, const T& msg)
		{
			bool use_default = sm_LogToDefaultLogger.load(std::memory_order_acquire);
			auto logger = (use_default) ? spdlog::default_logger() : spdlog::get(logger_name);
			assert(logger != nullptr);
			logger->log(log_level, msg);
		}

		template <typename ... Args>
		inline static void FormattedLogIt(const std::string& logger_name, 
			const spdlog::level::level_enum& log_level, const std::string_view fmt, Args... args)
		{
			ctx::LogIt(logger_name, log_level, std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
		}
		template <typename ... Args>
		inline static void AutoFormattedLogIt(const std::string& logger_name,
			const spdlog::level::level_enum& log_level, const std::string_view fmt, Args... args)
		{

		}

		inline static ctx& Get()
		{
			static std::unique_ptr<ctx> sm_Instance(new ctx);
			return *sm_Instance;
		}

	private:
		inline static std::mutex sm_CtxDataLock;
		inline static std::atomic_bool sm_LogToDefaultLogger{true};
	};

	inline void rename_default_logger(const char* new_name)
	{
		// Replace default_logger name.
		auto old_default_logger = spdlog::default_logger();
		auto new_default_logger = old_default_logger->clone(new_name);
		spdlog::set_default_logger(new_default_logger);
	}

	inline static void default_initialize(void) 
	{
		ctx::Get();

		//we need to rename default logger
		rename_default_logger("simperf");

		auto file_log = std::make_shared<spdlog::sinks::basic_file_sink_st>("simperf.log");

		spdlog::default_logger()->sinks().push_back(file_log);

		spdlog::default_logger()->set_level(sp_log_level::trace);
		spdlog::default_logger()->flush_on(sp_log_level::trace);

    }

	inline static void initialize_from_config(const char* path) 
	{
		ctx::Get(); // Initialize ctx
		spdlog_setup::from_file(path);
    }

    inline void initialize_from_log_specs() 
	{
		ctx::Get();

	}

	





	template <typename T, typename = void>
	struct HasStreamOperator : std::false_type {};
	
	template <typename T>
	struct HasStreamOperator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>> : std::true_type {};

	struct ProfiledArgBase
	{
		virtual ~ProfiledArgBase() {}
	public:
		virtual std::string to_string() = 0;
	};

	template<class T>
	struct ProfiledArg : ProfiledArgBase
	{
		T& _value;
		static_assert(HasStreamOperator<T>::value, "T must have the << operator defined");
	public:
		ProfiledArg(T& v) : _value(v) {}
		virtual std::string to_string() override {
			return std::format("{}", _value);
		}
	};

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
		template<typename ...Args>
		InstrumentationTimer(const char* name, Args&&... args)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
			AddArgs(std::forward<Args>(args)...);
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		template <typename ...Args>
		void AddArgs(Args&& ... inputs) {
			int i = 0;
			auto logger = spdlog::get("test");
			([&]
				{
					++i;
					logger->trace("Arg: {0} val: {1}", i, inputs);
					m_ProfiledArgs.insert({ 
						std::to_string(i), 
						std::make_shared<ProfiledArg<typename std::remove_reference<decltype(inputs)>::type>>(inputs)});
				} (), ...);
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			auto logger = spdlog::get("test");
			//logger->trace("{0} on thread {1} took {2}", m_Name, std::this_thread::get_id(), elapsedTime);
			//SIMPERF_LOG_PROFILE("{0} on thread {1} took {2}", m_Name, std::this_thread::get_id(), elapsedTime);
			Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });
			m_Stopped = true;
			std::stringstream ss;
			ss << std::this_thread::get_id();
			uint64_t id = std::stoull(ss.str());
			logger->trace("{0} on thread {1} took {2}", m_Name, id, elapsedTime);
			for (const auto& kv : m_ProfiledArgs) {

				logger->trace("Arg: {0} val: {1}", kv.first, kv.second->to_string());
			}
		}

	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		std::map<std::string, std::shared_ptr<ProfiledArgBase>> m_ProfiledArgs;
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
#define SIMPERF_PROFILE_SCOPE_LINE2(name, line, ...) constexpr auto fixedName##line = ::simperf::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
												   ::simperf::InstrumentationTimer timer##line(fixedName##line.Data, __VA_ARGS__)

#define SIMPERF_PROFILE_SCOPE_LINE(name, line, ...) SIMPERF_PROFILE_SCOPE_LINE2(name, line, __VA_ARGS__)
#define SIMPERF_PROFILE_SCOPE(name, ...) SIMPERF_PROFILE_SCOPE_LINE(name, __LINE__, __VA_ARGS__)
#define SIMPERF_PROFILE_FUNCTION(...) SIMPERF_PROFILE_SCOPE(SIMPERF_FUNC_SIG, __VA_ARGS__)

#else
#define SIMPERF_PROFILE_BEGIN_SESSION(name, filepath)
#define SIMPERF_PROFILE_END_SESSION()
#define SIMPERF_PROFILE_SCOPE(name)
#define SIMPERF_PROFILE_FUNCTION()
#endif
}