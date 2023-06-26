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
}

void register_static_example(const std::vector<spdlog::sink_ptr>& sinks)
{
	std::shared_ptr<spdlog::logger> core = std::make_shared<spdlog::logger>("core", std::begin(sinks), std::end(sinks));
	std::shared_ptr<spdlog::logger> client = std::make_shared<spdlog::logger>("client", std::begin(sinks), std::end(sinks));

	core->set_level(spdlog::level::trace);
	core->flush_on(spdlog::level::trace);

	assert(simperf::Log::RegisterStatic({ core, client }) == simperf::register_result::ok);
	
	int x = 0;
	CORE_TRACE("hi from core {0}", x);
	CORE_DEBUG("hi from core {0}", x);
	CORE_INFO("hi from core {0}", x);
	CORE_WARN("hi from core {0}", x);
	CORE_ERROR("hi from core {0}", x);
	CORE_CRITICAL("hi from core {0}", x);
}

void register_dynamic_example(const std::vector<spdlog::sink_ptr>& sinks)
{
	std::shared_ptr<spdlog::logger> dynamic = std::make_shared<spdlog::logger>("dynamic", std::begin(sinks), std::end(sinks));

	dynamic->set_level(spdlog::level::trace);
	dynamic->flush_on(spdlog::level::trace);

	assert(simperf::Log::RegisterDynamic(dynamic) == simperf::register_result::ok);

	int x = 1;
	SIMPERF_TRACE("dynamic", "hi from dynamic {0}", x);

}

int main()
{
	std::vector<spdlog::sink_ptr> sinks;
	make_sinks(sinks);

	register_static_example(sinks);
	register_dynamic_example(sinks);

	return 0;
}