#include "../include/simperf.hpp"

#define CORE_TRACE(...)		   SP_STATIC_LOG_("core", spdlog::level::trace, __VA_ARGS__)
#define CORE_DEBUG(...)		   SP_STATIC_LOG_("core", spdlog::level::debug, __VA_ARGS__)
#define CORE_INFO(...)		   SP_STATIC_LOG_("core", spdlog::level::info, __VA_ARGS__)
#define CORE_WARN(...)		   SP_STATIC_LOG_("core", spdlog::level::warn, __VA_ARGS__)
#define CORE_ERROR(...)		   SP_STATIC_LOG_("core", spdlog::level::err, __VA_ARGS__)
#define CORE_CRITICAL(...)	   SP_STATIC_LOG_("core", spdlog::level::critical, __VA_ARGS__)

#define CLIENT_TRACE(...)	   SP_STATIC_LOG_("client", spdlog::level::trace, __VA_ARGS__)
#define CLIENT_DEBUG(...)	   SP_STATIC_LOG_("client", spdlog::level::debug, __VA_ARGS__)
#define CLIENT_INFO(...)	   SP_STATIC_LOG_("client", spdlog::level::info, __VA_ARGS__)
#define CLIENT_WARN(...)	   SP_STATIC_LOG_("client", spdlog::level::warn, __VA_ARGS__)
#define CLIENT_ERROR(...)	   SP_STATIC_LOG_("client", spdlog::level::err, __VA_ARGS__)
#define CLIENT_CRITICAL(...)   SP_STATIC_LOG_("client", spdlog::level::critical, __VA_ARGS__)

void register_static_example()
{
	std::vector<spdlog::sink_ptr> sinks;

	sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("core.log", true));

	sinks[0]->set_pattern("%^[%T] %n: %v%$");
	sinks[1]->set_pattern("[%T] [%l] %n: %v");

	std::shared_ptr<spdlog::logger> core = std::make_shared<spdlog::logger>("core", std::begin(sinks), std::end(sinks));
	std::shared_ptr<spdlog::logger> client = std::make_shared<spdlog::logger>("client", std::begin(sinks), std::end(sinks));

	core->set_level(spdlog::level::trace);
	core->flush_on(spdlog::level::trace);

	client->set_level(spdlog::level::trace);
	client->flush_on(spdlog::level::trace);

	assert(simperf::Log::RegisterStatic({ core, client }) == simperf::register_result::ok);
}

int main()
{
	register_static_example();
	int x = 0;

	CORE_TRACE("hi from core {0}", x);
	CORE_DEBUG("hi from core {0}", x);
	CORE_INFO("hi from core {0}", x);
	CORE_WARN("hi from core {0}", x);
	CORE_ERROR("hi from core {0}", x);
	CORE_CRITICAL("hi from core {0}", x);

	CLIENT_TRACE("hi from client {0}", x);
	CLIENT_DEBUG("hi from client {0}", x);
	CLIENT_INFO("hi from client {0}", x);
	CLIENT_WARN("hi from client {0}", x);
	CLIENT_ERROR("hi from client {0}", x);
	CLIENT_CRITICAL("hi from client {0}", x);

	return 0;
}