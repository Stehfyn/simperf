#include "../include/simperf2.hpp"

void test_initialize() {
	::simperf::initialize_from_config();
}
int main() {
	//::simperf::initialize_from_config()
	//or
	//::simperf::ctx::RegisterStaticLoggers(log_specs)

	return 0;
}