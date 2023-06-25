#include "../include/simperf.hpp"

#define CORE_TRACE(...) SP_STATIC_TRACE_("core", spdlog::level::trace, __VA_ARGS__)

void register_core()
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("core.log", true));
	logSinks[0]->set_pattern("%^[%T] %n: %v%$");
	logSinks[1]->set_pattern("[%T] [%l] %n: %v");

	std::shared_ptr<spdlog::logger> core = std::make_shared<spdlog::logger>("core", begin(logSinks), end(logSinks));
	core->set_level(spdlog::level::trace);
	core->flush_on(spdlog::level::trace);

	using ls = simperf::logger_specification;
	ls core_spec{ logSinks, core };

	simperf::Log::RegisterStatic({ core_spec });
}

int main()
{
	register_core();
	CORE_TRACE("hi from core {0}", "0");
	return 0;
}