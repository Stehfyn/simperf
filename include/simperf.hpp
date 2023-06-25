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
	struct logger_specification {
		std::vector<spdlog::sink_ptr> sinks;
		std::shared_ptr<spdlog::logger> logger;
	};

	typedef std::vector<logger_specification> log_list;

	enum class register_result {
		failed = 0,
		ok,
	};

	inline bool operator!(register_result rr) {
		return rr == static_cast<register_result>(0);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>

	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	//namespace {
	//	template<typename Arg, typename... Args>
	//	LogIt_()
	//}

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
					for (const auto& log_spec : log_list_) {
						auto result = RegisterLogger(log_spec);
						if (!result) {
							return register_result::failed;
						}
						else {
							s_StaticLoggerRegistry.insert({ log_spec.logger->name(), log_spec.logger });
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

		template<typename ... Args>
		static void LogIt(const std::string& logger_name, const type& logger_type, const spdlog::level::level_enum& log_level, Args&&... args) {
			auto logger = GetLogger(logger_name, logger_type);
			logger == nullptr ? void(0) : LogTo(logger, log_level, std::forward<Args>(args)...);
		}

	private:
		static register_result RegisterLogger(const logger_specification& log_spec) {
			try {
				spdlog::register_logger(log_spec.logger);
				return register_result::ok;
			}
			catch (spdlog::spdlog_ex& e) {
				std::cerr << e.what() << std::endl;
				return register_result::failed;
			}
		}

		template<typename ... Args>
		static void LogTo(Ref<spdlog::logger> logger, const spdlog::level::level_enum& log_level, Args&&... args) {
			using namespace spdlog::level;
			switch (log_level)
			{
			case trace: {
				logger->trace(std::forward<Args>(args)...);
				break;
			}
			case debug: {
				logger->debug(std::forward<Args>(args)...);
				break;
			}
			case info: {
				logger->info(std::forward<Args>(args)...);
				break;
			}
			case warn: {
				logger->warn(std::forward<Args>(args)...);
				break;
			}
			case err: {
				logger->error(std::forward<Args>(args)...);
				break;
			}
			case critical: {
				logger->critical(std::forward<Args>(args)...);
				break;
			}
			default:
				break;
			}
		}

	private:
		inline static std::mutex s_RegisterLock;
		inline static std::atomic_bool s_StaticRegistryInitialized;
		inline static std::unordered_map<std::string, Ref<spdlog::logger>> s_StaticLoggerRegistry;
	};

#define SP_STATIC_TRACE_(logger_name, log_level, ...) ::simperf::Log::LogIt(logger_name, ::simperf::Log::type::static_logger, log_level, __VA_ARGS__)
#define SP_DYNAMIC_TRACE_(logger_name, log_level, ...) ::simperf::Log::LogIt(logger_name, ::simperf::Log::type::dynamic_logger, log_level, __VA_ARGS__)
}