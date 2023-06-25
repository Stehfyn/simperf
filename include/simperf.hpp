#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <iostream>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace simperf
{
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

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>

	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	class Log {
	public:
		enum class type {
			static_logger,
			dynamic_logger
		};

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
							s_StaticLoggerRegistry.insert({ logger->name(), logger });
						}
					}
				}
			}
			return register_result::ok;
		}

		static register_result RegisterDynamic(const log_list& log_list_) {
			for (const auto& log_spec : log_list_) {
				auto result = RegisterLogger(log_spec);
				if (!result) {
					return register_result::failed;
				}
			}
			return register_result::ok;
		}

		static void Unregister(const std::string& name, const type& logger_type) {
			if (logger_type == ::simperf::Log::type::static_logger) {
				if (s_StaticLoggerRegistry.erase(name)) {
					spdlog::drop(name);
				}
			}
			else {
				spdlog::drop(name);
			}
		}

		static Ref<spdlog::logger> GetLogger(const std::string& logger_name, const type& logger_type) {
			if (logger_type == type::static_logger) {
				auto found = s_StaticLoggerRegistry.find(logger_name);
				return found == s_StaticLoggerRegistry.end() ? nullptr : found->second;
			}
			else {
				return spdlog::get(logger_name);
			}
		}

	private:
		static register_result RegisterLogger(const logger& log_spec) {
			try {
				spdlog::register_logger(log_spec);
				return register_result::ok;
			}
			catch (spdlog::spdlog_ex& e) {
				std::cerr << e.what() << std::endl;
				return register_result::failed;
			}
		}

	private:
		inline static std::mutex s_RegisterLock;
		inline static std::atomic_bool s_StaticRegistryInitialized;
		inline static std::unordered_map<std::string, Ref<spdlog::logger>> s_StaticLoggerRegistry;
	};

#define SP_LOG_IT_(logger_name, logger_type, log_level, ...)  \
	{ \
		simperf::Ref<spdlog::logger> logger = simperf::Log::GetLogger(logger_name, logger_type); \
		logger == nullptr ? void(0) : \
		log_level == ::spdlog::level::trace ? logger->trace(__VA_ARGS__) : \
		log_level == ::spdlog::level::debug ? logger->debug(__VA_ARGS__) : \
		log_level == ::spdlog::level::info ? logger->info(__VA_ARGS__) : \
		log_level == ::spdlog::level::warn ? logger->warn(__VA_ARGS__) : \
		log_level == ::spdlog::level::err ? logger->error(__VA_ARGS__) : \
		log_level == ::spdlog::level::critical ? logger->critical(__VA_ARGS__) : \
		void(0); \
	} 

#define SP_STATIC_LOG_(logger_name, log_level, ...) SP_LOG_IT_(logger_name, ::simperf::Log::type::static_logger, log_level, __VA_ARGS__)
#define SP_DYNAMIC_LOG_(logger_name, log_level, ...) SP_LOG_IT_(logger_name, ::simperf::Log::type::dynamic_logger, log_level, __VA_ARGS__)

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
}