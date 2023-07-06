#define SIMPERF_ENABLE
#include "../include/simperf2.hpp"
#include <mutex>
#include <condition_variable>

std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;
#define TEST_ASSERT(check, left, right) auto a = ::simperf::Assertion(left != right, left, right, ::simperf::make_array(std::string("simperf"))); if (a) { SIMPERF_DEBUGBREAK(); }
#define TEST_ASSERT2(check, left, right) auto a2 = ::simperf::Assertion(left != right, left, right); if (a2) { SIMPERF_DEBUGBREAK(); }
void worker_thread()
{
    using namespace std::chrono_literals;

    SIMPERF_PROFILE_FUNCTION(ready, processed, data);

    auto logger = spdlog::get("test");

    std::this_thread::sleep_for(3s);

    // Wait until main() sends data
    std::unique_lock lk(m);
    cv.wait(lk, [] {return ready; });

    // after the wait, we own the lock.
    //logger->trace("Worker thread is processing data");

    data += " after processing";

    // Send data back to main()
    processed = true;
    //logger->trace("Worker thread signals data processing completed");

    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lk.unlock();
    cv.notify_one();
}

void test_mt() {
    
	::simperf::initialize_from_config("log.conf");
    SIMPERF_PROFILE_BEGIN_SESSION("test", "test-session.json");

    std::thread worker(worker_thread);
    auto logger = spdlog::get("test");

    data = "Example data";
    // send data to the worker thread
    {
        std::lock_guard lk(m);
        ready = true;
        logger->trace("main() signals data ready for processing");
    }
    cv.notify_one();

    // wait for the worker
    {
        std::unique_lock lk(m);
        cv.wait(lk, [] {return processed; });
    }

    worker.join();
    logger->trace("Back in main(), data = {0}:", data);
    simperf::ctx::FormattedLogIt("", spdlog::level::level_enum::critical, "{0} {1} {2}", 0, 1, 2);
    simperf::ctx::LogIt("", spdlog::level::level_enum::critical, "Hello :)");
    SIMPERF_PROFILE_END_SESSION();
}

void test_st()
{
    ::simperf::default_initialize();
    
    simperf::ctx::FormattedLogIt("", spdlog::level::level_enum::trace, "{0} {1} {2}", 0, 1, 2);
    simperf::ctx::LogIt("", spdlog::level::level_enum::trace, "Hello :)");
    SIMPERF_TRACE("simperf", "hi");
}

void test_assert()
{
    int x = 1;
    int y = 1;

#ifdef _DEBUG
    ::simperf::ctx::SetVariableShouldThrowOn(true);
#else
    ::simperf::ctx::SetVariableShouldThrowOn(false);
#endif

    SIMPERF_ASSERT_NEQ_NO_THROW(69, 69, "simperf", "ASSERT 69 equals 69!", x, y);
    //SIMPERF_ASSERT_NEQ_VARIABLE_THROW(96, 96, "simperf", "96 equals 96! {} {}", x, y);
    //SIMPERF_ASSERT_NEQ(1, 1, "simperf", "1 equals 1!", x, y);

    std::string test_str("hello");
    auto ss = ::simperf::SmartString("hello");
    auto ss2 = ::simperf::SmartString(test_str);
    auto ss3 = ::simperf::SmartString(std::string_view("string_view of char lit to smart str to string :)"));
    auto ams = ::simperf::AssertionMsgSpec();

    std::string constexpr_test;
    for (int i = 0; i < 10; ++i) {
        constexpr_test.push_back(62 + i);
    }
	::simperf::ctx::AddTag("simperf");
	::simperf::ctx::AddTag(std::string("simperf1"));
	::simperf::ctx::AddTag(ss);
	::simperf::ctx::AddTag(ss3);
	::simperf::ctx::AddTag(test_str + "ur mom");

    simperf::ctx::FormattedLogIt("", spdlog::level::level_enum::warn, 
        ams.m_MsgPrefix.val, "\"a==b\"", std::source_location::current().line(), 
        std::source_location::current().file_name());

	//TEST_ASSERT(ss != ss2, ss, ss2);
	//TEST_ASSERT2(ss != ss2, ss, ss2);
    ::simperf::ctx::SetAssertionTypeStatus(::simperf::AssertionType::Fatal, false);
    auto a3 = ::simperf::Assertion(ss != ss2, ss, ss2, ::simperf::make_array(constexpr_test, "hi", test_str.c_str(), R"(literl)"));
}

int main()
{
    test_st();
    try {
        test_assert();
		for (const auto& tag : ::simperf::ctx::Tags()) {
			std::cout << "tag: " << tag << std::endl;
		}
    }
    catch (std::exception& e) {
        auto logger = spdlog::get("simperf");
        logger->warn("exception caught");
    }
    return 0;
}