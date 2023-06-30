// Uncomment to force enable simperf in release
#if !defined(_DEBUG)
	#define SIMPERF_ENABLE
#endif

// Uncomment to force disable simperf in debug
//#if defined(_DEBUG)
//	#define SIMPERF_DISABLE
//#endif

#include "../include/simperf.hpp"

#define CORE_TRACE(...)		   SIMPERF_TRACE("core", __VA_ARGS__)
#define CORE_DEBUG(...)		   SIMPERF_DEBUG("core", __VA_ARGS__)
#define CORE_INFO(...)		   SIMPERF_INFO("core", __VA_ARGS__)
#define CORE_WARN(...)		   SIMPERF_WARN("core", __VA_ARGS__)
#define CORE_ERROR(...)		   SIMPERF_ERROR("core", __VA_ARGS__)
#define CORE_CRITICAL(...)	   SIMPERF_CRITICAL("core", __VA_ARGS__)

void make_sinks(std::vector<spdlog::sink_ptr>& sinks)
{
	sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("core.log", true));

	sinks[0]->set_pattern("%^[%T] %n: %v%$");
	sinks[1]->set_pattern("[%T] [%l] %n: %v");
	//SIMPERF_ASSERT_WARGS(1 == 2);
}

void register_static_example(const std::vector<spdlog::sink_ptr>& sinks)
{
	std::shared_ptr<spdlog::logger> core = std::make_shared<spdlog::logger>("core", std::begin(sinks), std::end(sinks));
	std::shared_ptr<spdlog::logger> client = std::make_shared<spdlog::logger>("client", std::begin(sinks), std::end(sinks));

	core->set_level(spdlog::level::trace);
	core->flush_on(spdlog::level::trace);

	SIMPERF_ASSERT(simperf::Log::RegisterStatic({ core, client }) == simperf::register_result::ok, "LOL LOSER");
	simperf::Log::SetErrorLogger("core");
	simperf::Log::SetDebugLogger("core");

	::simperf::Log::LogIt("core", "{1}", 10);

	int x = 0;
	CORE_TRACE("hi from core {0}", x);
	CORE_DEBUG("hi from core {0}", x);
	CORE_INFO("hi from core {0}", x);
	CORE_WARN("hi from core {0}", x);
	CORE_ERROR("hi from core {0}", x);
	CORE_CRITICAL("hi from core {0}", x);
}

int main()
{
	SIMPERF_PROFILE_BEGIN_SESSION("test", "simperf-test.json");
	std::vector<spdlog::sink_ptr> sinks;
	make_sinks(sinks);

	register_static_example(sinks);

	std::cout << "end" << std::endl;
	return 0;
}