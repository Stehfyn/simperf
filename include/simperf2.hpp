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
#include <functional>
#include <variant>
#include <source_location>
#include <stack>
#include <array>
#include <windows.h>

#if defined(SIMPERF_LIB)
	//#include <spdlog/spdlog.h>
	//#include <spdlog/common.h>
	//#include <spdlog/fmt/ostr.h>
	//#include <spdlog/sinks/basic_file_sink.h>
	//#include <spdlog/sinks/stdout_color_sinks.h>
//#define SPDLOG_USE_STD_FORMAT
	#include <spdlog_setup/conf.h>
	//#include <spdlog/fmt/bundled/color.h>
#else
	#include "../external/spdlog/include/spdlog/spdlog.h"
	#include "../external/spdlog/include/spdlog/common.h"
	#include "../external/spdlog/include/spdlog/fmt/ostr.h"
	#include "../external/spdlog/include/spdlog/sinks/basic_file_sink.h"
	#include "../external/spdlog/include/spdlog/sinks/stdout_color_sinks.h"
	#include "../external/spdlog_setup/include/spdlog_setup/conf.h"
#endif
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/bundled/color.h>

#include "details/log-impl.h"
#include "details/assert-impl.h"

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

		inline static bool VariableShouldThrow(void)
		{
			return sm_VariableNoThrowActive.load(std::memory_order_acquire);
		}

		inline static void SetVariableShouldThrowOn(bool flag = false)
		{
			sm_VariableNoThrowActive.store(flag, std::memory_order_release);
		}

		inline static ctx& Initialize()
		{
			static void* sm_Initialized(nullptr);
			assert(sm_Initialized == nullptr);
			static std::unique_ptr<ctx> sm_Instance(new ctx);
			_Initialize();
			sm_Initialized = static_cast<void*>(&sm_Instance);
			return *sm_Instance;
		}

		inline static std::vector<std::string_view> Tags(void) {
			std::vector<std::string_view> tags;
			for (const auto& kv : sm_TagStatusMap) {
				tags.push_back(kv.first.val);
			}
			return tags;
		}

		template <typename T>
		inline static void AddTag(const T& tag, bool enabled = true) {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			sm_TagStatusMap.insert({ InstrumentTag(tag), enabled});
		}

		inline static std::string_view GetDefaultTag(void) {
			return sm_DefaultTag.val;
		}

		inline static void SetAssertionTypeStatus(const AssertionType& type, bool enabled = true) {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			auto it = sm_AssertionTypeStatusMap.find(type);
			assert(it != sm_AssertionTypeStatusMap.end());
			it->second = enabled;
		}

		inline static bool GetAssertionTypeStatus(const AssertionType& type) {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			auto it = sm_AssertionTypeStatusMap.find(type);
			assert(it != sm_AssertionTypeStatusMap.end());
			return it->second;
		}

		template <typename T>
		inline static void SetInstrumentTagStatus(const T& tag, bool enabled = true) {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			auto it = sm_TagStatusMap.find(InstrumentTag(tag));
			assert(it != sm_TagStatusMap.end());
			it->second = enabled;
		}

		template<typename T>
		inline static bool GetInstrumentTagStatus(const T& tag) {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			auto it = sm_TagStatusMap.find(InstrumentTag(tag));
			assert(it != sm_TagStatusMap.end());
			return it->second;
		}

		void SetGlobalAssertionStatus(bool enabled = true) {
			sm_VariableNoThrowActive.store(enabled, std::memory_order_release);
		}

		inline static bool GetGlobalAssertionStatus(void) {
			return sm_VariableNoThrowActive.load(std::memory_order_acquire);
		}

	private:
		inline static void _Initialize(void) {
			sm_AssertionTypeStatusMap = {{ AssertionType::ExplicitNoThrow, true },
										 { AssertionType::VariableThrow, true } ,
										 { AssertionType::Throw, true },
										 { AssertionType::Fatal, true }};
		}
	private:
		inline static InstrumentTag sm_DefaultTag{ "simperf" };
		
		inline static std::mutex sm_CtxDataLock;
		inline static std::atomic_bool sm_LogToDefaultLogger{true};
		inline static std::atomic_bool sm_VariableNoThrowActive{false};

		inline static std::stack<LoggerTag> sm_LoggerTagStack;
		inline static std::stack<InstrumentTag> sm_InstrumentTagStack;

		inline static std::atomic_bool sm_GlobalAssertionSwitch{true};
		inline static std::unordered_map<AssertionType, bool> sm_AssertionTypeStatusMap;
		inline static std::unordered_map<InstrumentTag, 
			bool, std::hash<InstrumentTag>> sm_TagStatusMap;
	};

#pragma region inlinehelpers
	inline void rename_default_logger(const char* new_name)
	{
		// Replace default_logger name.
		auto old_default_logger = spdlog::default_logger();
		auto new_default_logger = old_default_logger->clone(new_name);
		spdlog::set_default_logger(new_default_logger);
		
	}

	inline static void default_initialize(void) 
	{
		ctx::Initialize();

		//we need to rename default logger
		rename_default_logger("simperf");

		auto file_log = std::make_shared<spdlog::sinks::basic_file_sink_st>("simperf.log");

		spdlog::default_logger()->sinks().push_back(file_log);

		spdlog::default_logger()->set_level(sp_log_level::trace);
		spdlog::default_logger()->flush_on(sp_log_level::trace);
    }

	inline static void initialize_from_config(const char* path) 
	{
		ctx::Initialize(); // Initialize ctx
		spdlog_setup::from_file(path);
    }

    inline void initialize_from_log_specs() 
	{
		ctx::Initialize();

	}
#pragma endregion inlinehelpers


	
#pragma region Assertions
	template <typename T1 = std::string_view,
		typename T2 = std::string_view, typename T3 = std::string_view>
	struct AssertionMsgSpec {
		SmartString<T1> m_Msg;
		SmartString<T2> m_MsgPrefix;
		SmartString<T3> m_Expression;

		AssertionMsgSpec(const T1& msg, const T2& msg_prefix, const T3& expression) :
			m_Msg(msg), m_MsgPrefix(msg_prefix), m_Expression(expression)
		{
		}

		AssertionMsgSpec(std::string_view msg = ASSERT_FMTMSG,
			std::string_view msg_prefix = ASSERT_PREFIX, std::string_view expression = "") :
			m_Msg(msg), m_MsgPrefix(msg_prefix), m_Expression(expression)
		{
		}
	};

	template <typename T1 = std::string_view,
		typename T2 = std::string_view, typename T3 = std::string_view>
	AssertionMsgSpec(const T1& msg, const T2& msg_prefix, const T3& expression)
		->AssertionMsgSpec<T1, T2, T3>;

	template <typename T1 = std::any, typename T2 = std::any>
	struct AssertionSpec {
		AssertionType Type;
		AssertionMsgSpec<> MsgSpec;
		AssertionControlFlags ControlFilter;
		AssertionLogLevelTargets LogLevelTargets;
		std::function<bool(T1, T2)> Override;

		AssertionSpec(AssertionType type = AssertionType::Fatal,
			AssertionControlFlags control_filter = AssertionControlFlags::Global |
			AssertionControlFlags::Type | AssertionControlFlags::Tag,
			AssertionMsgSpec<> msg_spec = {},
			AssertionLogLevelTargets log_level_targets = get_default_assertion_log_level_targets(),
			std::function<bool(T1, T2)> _override =
			[](const T1& lhs, const T2& rhs) -> bool { return false; })
			: Type(type), ControlFilter(control_filter), LogLevelTargets(log_level_targets),
			Override(_override)
		{
		}
	};

	template <typename T1 = std::any, typename T2 = std::any>
	AssertionSpec(AssertionType, AssertionControlFlags, AssertionLogLevelTargets,
		std::function<bool(T1, T2)>)
		->AssertionSpec<T1, T2>;

	template <size_t N>
	class AssertionBase {
	public:
		explicit operator bool() const {
			//m_AssertionSpec.
			return !m_Check;
		}

		void Flush() {

		}

		~AssertionBase() {
			auto logger = spdlog::get("simperf");
			logger->warn("assertion destructor called");
		}

	protected:
		AssertionBase(bool check = false, 
			std::array<SmartString<std::string>, N> tags = make_array(std::string(::simperf::ctx::GetDefaultTag())),
			AssertionSpec<> spec = {}) :
			m_Check(check), m_FailedCheck(false), m_Tags(tags), m_AssertionSpec(spec)
		{
			using acf = AssertionControlFlags;

			if (!check) {
				if (!!(m_AssertionSpec.ControlFilter & acf::Override)) {
					//m_FailedCheck = m_AssertionSpec.Override();
					m_FailedCheck = true;
				}

				else if (!!(m_AssertionSpec.ControlFilter & acf::Global)) {
					if (::simperf::ctx::GetGlobalAssertionStatus()) {
						bool listens_to_type = !!(m_AssertionSpec.ControlFilter & acf::Type);
						bool listens_to_tag =  !!(m_AssertionSpec.ControlFilter & acf::Tag);

						if (listens_to_type && listens_to_tag) {
							//check if type is active
							bool type_is_active = 
								::simperf::ctx::GetAssertionTypeStatus(m_AssertionSpec.Type);

							//bool tags_are_active = true;
							m_FailedCheck = type_is_active;
						}
					}


					//check if asserts are globally active
					//check if this type of assert is active
					//check if asserts with this tag are active
				}
			}
		}

		std::function<bool()> m_ThrowIf;
		std::string m_Logger;

		AssertionSpec<> m_AssertionSpec;
		std::source_location m_SourceLocation;
		std::array<SmartString<std::string>, N> m_Tags;

	private:
		bool m_Check;
		bool m_FailedCheck;
	};

	template <size_t N>
	AssertionBase(bool, std::array<SmartString<std::string>, N>, AssertionSpec<>)->AssertionBase<N>;

	template <typename T1, typename T2, size_t N>
	class Assertion : public AssertionBase<N> {
	public:
		Assertion(bool check, const T1& lhs, const T2& rhs, std::array<SmartString<std::string>, N> tags =
			make_array(std::string(::simperf::ctx::GetDefaultTag())), AssertionSpec<> spec = {},
			std::source_location source_loc = std::source_location::current())
			: AssertionBase<N>(check, tags, spec)
		{
		}
	};

	template <typename T1, typename T2, size_t N>
	Assertion(bool check, const T1& lhs, const T2& rhs, std::array<SmartString<std::string>, N> tags =
		make_array(std::string(::simperf::ctx::GetDefaultTag())),
		AssertionSpec<T1, T2> spec = {}, std::source_location source_loc =
		std::source_location::current())->Assertion<T1, T2, N>;

	template <typename T1, typename T2>
	Assertion(bool check, const T1& lhs, const T2& rhs)->Assertion<T1, T2, 1>;

#pragma endregion Assertions

	//template <typename T, typename... Ts>
	//constexpr std::array<T, 1 + sizeof...(Ts)> make_array(T&& first, Ts&&... args) {
	//	return std::array<T, 1 + sizeof...(Ts)>{std::forward<T>(first), std::forward<Ts>(args)...};
	//}
	


#pragma region profiling
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
#pragma endregion profiling
