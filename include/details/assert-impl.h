namespace simperf {
#pragma region SmartString
template <typename StringType = const char *> class SmartString {
public:
  StringType val;

  constexpr SmartString(const StringType &str) : val(str) {}
  constexpr SmartString() : val("") {}

  SmartString(const SmartString<StringType> &other) : val(other.val) {}
  SmartString(SmartString<StringType> &&other) noexcept : val(std::move(other.val)) {}

  template <typename OtherStringType>
  SmartString(const SmartString<OtherStringType> &other) : val(other.val) {}

  template <typename OtherStringType>
  SmartString(SmartString<OtherStringType> &&other) noexcept : val(std::move(other.val)) {}
};

template <typename StringType> SmartString(const StringType &str) -> SmartString<StringType>;

template <std::size_t N> SmartString(const char (&)[N]) -> SmartString<std::string_view>;

template <typename lht, typename rht>
bool operator==(const SmartString<lht> &lhs, const SmartString<rht> &rhs) {
  return std::string_view(lhs.val) == std::string_view(rhs.val);
}

typedef SmartString<std::string> LoggerTag;
typedef SmartString<std::string> InstrumentTag;
} // namespace simperf

namespace std {
template <> struct hash<simperf::SmartString<std::string>> {
  std::size_t operator()(const simperf::SmartString<std::string> &s) const {
    return std::hash<std::string_view>{}(s.val);
  }
};
} // namespace std
namespace simperf {
#pragma endregion SmartString
// Expression string, Provide source_location
static constexpr std::string_view ASSERT_PREFIX("ASSERT: Expression \"{}\" failed at line {} in {}");
static constexpr std::string_view ASSERT_FMTMSG("\n{}: {}\n{}: {}");

enum class AssertionType { ValueCheck, ExplicitNoThrow, VariableThrow, Throw, Fatal };

#pragma region AssertionControlFlags
enum class AssertionControlFlags : int {
  Override = 1 << 0,
  Global = 1 << 1,
  Tag = 1 << 2,
  Type = 1 << 3
};

inline AssertionControlFlags operator~(const AssertionControlFlags &value) {
  return static_cast<AssertionControlFlags>(~static_cast<int>(value));
}

inline AssertionControlFlags operator|(const AssertionControlFlags &lhs,
                                       const AssertionControlFlags &rhs) {
  return static_cast<AssertionControlFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline AssertionControlFlags &operator|=(AssertionControlFlags &lhs,
                                         const AssertionControlFlags &rhs) {
  return lhs = lhs | rhs;
}

inline AssertionControlFlags operator&(const AssertionControlFlags &lhs,
                                       const AssertionControlFlags &rhs) {
  return static_cast<AssertionControlFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline AssertionControlFlags &operator&=(AssertionControlFlags &lhs,
                                         const AssertionControlFlags &rhs) {
  return lhs = lhs & rhs;
}

inline AssertionControlFlags operator^(const AssertionControlFlags &lhs,
                                       const AssertionControlFlags &rhs) {
  return static_cast<AssertionControlFlags>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

inline AssertionControlFlags &operator^=(AssertionControlFlags &lhs,
                                         const AssertionControlFlags &rhs) {
  return lhs = lhs ^ rhs;
}

inline bool operator!(const AssertionControlFlags &value) { return static_cast<int>(value) == 0; }
#pragma endregion AssertionControlFlags

#pragma region LogLevelTargeting
enum class ProfilingLogSource {
  FunctionSignature,
  ScopeValueTracking,
  ScopeTiming,
};

using sp_log_level = spdlog::level::level_enum;

typedef std::unordered_map<ProfilingLogSource, sp_log_level> ProfilingLogLevelTargets;
typedef std::unordered_map<AssertionType, sp_log_level> AssertionLogLevelTargets;

typedef std::pair<ProfilingLogLevelTargets, AssertionLogLevelTargets> DefaultLogLevelTargets;

ProfilingLogLevelTargets get_default_profiling_log_level_targets(void) {
  return {{ProfilingLogSource::FunctionSignature, sp_log_level::trace},
          {ProfilingLogSource::ScopeValueTracking, sp_log_level::debug},
          {ProfilingLogSource::ScopeTiming, sp_log_level::debug}};
}

AssertionLogLevelTargets get_default_assertion_log_level_targets(void) {
  return {{AssertionType::ValueCheck, sp_log_level::info},
          {AssertionType::ExplicitNoThrow, sp_log_level::warn},
          {AssertionType::VariableThrow, sp_log_level::err},
          {AssertionType::Throw, sp_log_level::err}, 
          {AssertionType::Fatal, sp_log_level::critical}};
  }

DefaultLogLevelTargets get_default_log_level_targets(void) {
  return {get_default_profiling_log_level_targets(), get_default_assertion_log_level_targets()};
}

std::unordered_map<std::string, DefaultLogLevelTargets> s_LogLevelTargets{
    {"simperf", get_default_log_level_targets()}};
#pragma endregion LogLevelTargeting

template <typename... Ts> constexpr auto make_array(Ts &&...args) {
  return std::array<SmartString<std::string>, sizeof...(Ts)>{
      SmartString<std::string>(std::forward<Ts>(args))...};
}
} // namespace simperf