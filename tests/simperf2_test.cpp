#define SIMPERF_ENABLE
#include "../include/simperf2.hpp"
#include <mutex>
#include <condition_variable>

std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;

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

    std::string tesst("hello");
    auto ss = ::simperf::SmartString("hello");
    auto ss2 = ::simperf::SmartString(tesst);

    auto ams = ::simperf::AssertionMsgSpec();
    
    simperf::ctx::FormattedLogIt("", spdlog::level::level_enum::warn, 
        ams.m_MsgPrefix.val, "\"a==b\"", std::source_location::current().line(), 
        std::source_location::current().file_name());

    auto a = ::simperf::Assertion(ss != ss2, ss, ss2);
    //std::cout << a.ShouldThrow() << std::endl;
    //auto ams = ::simperf::AssertionMsgSpec("hi", tesst, "hello");
    //auto ss = ::simperf::SmartString(tesst.data());
}

int main()
{
    test_st();
    try {
        test_assert();
    }
    catch (std::exception& e) {
        auto logger = spdlog::get("simperf");
        logger->warn("exception caught");
    }
    return 0;
}