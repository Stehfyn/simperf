#include "../include/simperf2.hpp"

void test_initialize() {
	::simperf::initialize_from_config("log.conf");
}

int main() {
	test_initialize();

	auto logger = spdlog::get("test");
	logger->trace("Hello World!");
	logger->debug("Hello World!");
	logger->info("Hello World!");
	logger->warn("Hello World!");
	logger->error("Hello World!");
	logger->critical("Hello World!");

	//::simperf::initialize_from_config()
	//or
	//::simperf::ctx::RegisterStaticLoggers(log_specs)

	return 0;
}