namespace simperf {

	enum class AssertionType {
		ExplicitNoThrow,
		VariableThrow,
		Throw,
		Fatal
	};

	enum class AssertionControlFlags {
		Global,
		AssertionType,
		AssertionTag,
		Callback,
		Override
	};

	enum class ProfilingLogSource {
		FunctionSignature,
		ScopeValueTracking,
		ScopeTiming,
	};

	enum class AssertionLogSource {
		ValueCheck,
		ExplicitNoThrow,
		VariableThrow,
		Throw
	};

	using sp_log_level = spdlog::level::level_enum;

	typedef std::unordered_map<ProfilingLogSource, sp_log_level> ProfilingLogLevelTargets;
	typedef std::unordered_map<AssertionLogSource, sp_log_level> AssertionLogLevelTargets;

	typedef std::pair<ProfilingLogLevelTargets, AssertionLogLevelTargets> DefaultLogLevelTargets;

	ProfilingLogLevelTargets get_default_profiling_log_level_targets(void) {
		return {{ProfilingLogSource::FunctionSignature,  sp_log_level::trace},
				{ProfilingLogSource::ScopeValueTracking, sp_log_level::debug},
				{ProfilingLogSource::ScopeTiming,        sp_log_level::debug}};
	}

	AssertionLogLevelTargets get_default_assertion_log_level_targets(void) {
		return {{AssertionLogSource::ValueCheck,      sp_log_level::info},
				{AssertionLogSource::ExplicitNoThrow, sp_log_level::warn},
				{AssertionLogSource::VariableThrow,   sp_log_level::err},
				{AssertionLogSource::Throw,           sp_log_level::critical} };
	}

	DefaultLogLevelTargets get_default_log_level_targets(void) {
		return {get_default_profiling_log_level_targets(),
				get_default_assertion_log_level_targets()};
	}

	std::unordered_map<std::string, DefaultLogLevelTargets> s_LogLevelTargets{
		{"simperf", get_default_log_level_targets() }
	};

	template<typename StringType> 
	class SmartString {
	public:
		StringType val;
		SmartString(const StringType& str) : val(str) {}
		SmartString(const char* default_init = "") : val(default_init) {}
	};

	template <typename StringType>
	SmartString(const StringType& str)->SmartString<StringType>;

	template <std::size_t N>
	SmartString(const char(&)[N])->SmartString<std::string_view>;

	template<typename lht, typename rht>
	bool operator==(const SmartString<lht>& lhs, const SmartString<rht>& rhs) {
		return std::string_view(lhs.val) == std::string_view(rhs.val);
	}

	// Expression string, Provide source_location
	static constexpr std::string_view ASSERT_PREFIX("ASSERT: Expression {} failed at line {} in {}");
	static constexpr std::string_view ASSERT_FMTMSG("");

	template <typename T1 = std::string_view, 
		typename T2 = std::string_view, typename T3 = std::string_view>
	struct AssertionMsgSpec {
		SmartString<T1> m_Msg;
		SmartString<T2> m_MsgPrefix;
		SmartString<T3> m_Expression;

		AssertionMsgSpec(
			const T1& msg,
			const T2& msg_prefix,
			const T3& expression) :
			m_Msg(msg), 
			m_MsgPrefix(msg_prefix), 
			m_Expression(expression) 
		{
		}

		AssertionMsgSpec(
			std::string_view msg = ASSERT_FMTMSG,
			std::string_view msg_prefix = ASSERT_PREFIX,
			std::string_view expression = "") :
			m_Msg(msg),
			m_MsgPrefix(msg_prefix),
			m_Expression(expression)
		{
		}
	};

	template <typename T1 = std::string_view,
		typename T2 = std::string_view, typename T3 = std::string_view>
	AssertionMsgSpec(const T1& msg, const T2& msg_prefix, const T3& expression)
		->AssertionMsgSpec<T1, T2, T3>;

	struct AssertionSpec {
		AssertionType Type;
		AssertionControlFlags ControlFilter;
		AssertionLogLevelTargets LogLevelTargets;
		std::function<bool()> Override;

		AssertionSpec(AssertionType type = AssertionType::Fatal,
			AssertionControlFlags control_filter = AssertionControlFlags::Global,
			AssertionLogLevelTargets log_level_targets = get_default_assertion_log_level_targets(),
			std::function<bool()> override = std::function<bool()>([]() -> bool { return false; }))
			: Type(type), ControlFilter(control_filter), LogLevelTargets(log_level_targets),
			Override(override)
		{
		}
	};

	class AssertionBase {
	public:
		~AssertionBase() {
			auto logger = spdlog::get("simperf");
			logger->warn("assertion destructor called");
		}

	protected:
		AssertionBase(bool check = false) :
			m_Check(check)
		{
		}

		bool m_Check;
		std::string m_Logger;

		std::function<bool()> m_ThrowIf;

		std::source_location m_SourceLocation;
		AssertionMsgSpec<> ams;
	};

	template <typename T1, typename T2>
	class Assertion : public AssertionBase {
	public:
		Assertion(bool check, const T1& lhs, const T2& rhs,
			AssertionSpec spec = {}, AssertionMsgSpec<> msg_spec = {},
			std::source_location source_loc = std::source_location::current())
			: AssertionBase(check)
		{
			AssertNow();
		}

		void AssertNow(void) {
			if (!m_Check) {
				SIMPERF_DEBUGBREAK();
			}
		}

		bool ShouldThrow() {
			return m_ThrowIf();
		}
	};

	template <typename T1, typename T2>
	Assertion(bool check, const T1& lhs, const T2& rhs,
		AssertionSpec spec = {}, AssertionMsgSpec<> msg_spec = {},
		std::source_location source_loc = std::source_location::current())->Assertion<T1, T2>;
}
