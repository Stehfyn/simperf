#pragma once

#include <mutex>
#include <memory>
#include <filesystem>

#if defined(SIMPERF_LIB)
	//#include <spdlog/spdlog.h>
	//#include <spdlog/common.h>
	//#include <spdlog/fmt/ostr.h>
	//#include <spdlog/sinks/basic_file_sink.h>
	//#include <spdlog/sinks/stdout_color_sinks.h>
	#include <spdlog_setup/conf.h>
#else
	#include "../external/spdlog/include/spdlog/spdlog.h"
	#include "../external/spdlog/include/spdlog/common.h"
	#include "../external/spdlog/include/spdlog/fmt/ostr.h"
	#include "../external/spdlog/include/spdlog/sinks/basic_file_sink.h"
	#include "../external/spdlog/include/spdlog/sinks/stdout_color_sinks.h"
	#include "../external/spdlog_setup/include/spdlog_setup/conf.h"
#endif

namespace simperf
{
	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>

	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	typedef Ref<spdlog::logger> logger;
	typedef std::vector<logger> log_list;

	void initialize_from_config(const std::filesystem::path& path = {});

	struct ctx_status
	{
		bool ok;
	};

	inline std::ostream& operator<<(std::ostream& os, const ctx_status& status) {
		os << "is ok: " << status.ok;
		return os;
	}

	class ctx {
	public:
		inline static void Initialize() {

		}

		inline static void RegisterStaticLoggers(const log_list& log_list_) {
			
		}

		inline static const ctx_status Status() {
			std::unique_lock<std::mutex> guard(sm_CtxDataLock);
			return sm_Status;
		}
		inline static ctx& Get()
		{
			static std::unique_ptr<ctx> sm_Instance(new ctx);
			return *sm_Instance;
		}
	private:
		inline static ctx_status sm_Status;
		inline static std::mutex sm_CtxDataLock;
	};

	static void initialize_from_config(const std::filesystem::path& path)
	{
		ctx::Get(); // Initialize ctx
	}
}