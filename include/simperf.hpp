#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>

constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

namespace simperf
{
	struct logger_specification {

	};

	enum class register_result {
		ok = 0,
		failed
	};

	typedef std::vector<logger_specification> log_list;

	class Log
	{
		enum class type
		{
			static_logger,
			dynamic_logger
		};

	public:

		static register_result RegisterStatic(const log_list& log_list_) {
			if (!s_StaticRegistryInitialized.load(std::memory_order_acquire)) {
				std::unique_lock<std::mutex> guard(s_RegisterLock);
				if (!s_StaticRegistryInitialized.load(std::memory_order_acquire)) {
					for (const auto& log_spec : log_list_) {
						auto result = RegisterLogger(log_spec);
						if (result != register_result::ok) return register_result::failed;
					}
				}
			}
		}

		//static register_result RegisterDynamic(const log_list& log_list_) {
		//	//auto spdlog::get()
		//}

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
		static register_result RegisterLogger(const logger_specification& log_spec) {
			return register_result::failed;
		}

	private:
		static std::unordered_map<std::string, Ref<spdlog::logger>> s_StaticLoggerRegistry;

		static std::mutex s_RegisterLock;
		static std::atomic_bool s_StaticRegistryInitialized;
	};
}