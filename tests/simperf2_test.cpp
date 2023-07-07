#define SIMPERF_ENABLE
#include "../include/simperf2.hpp"

void test_default_initialize();
void test_default_asserts();

int main() {
  try {
    test_default_initialize();
    test_default_asserts();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}

void test_default_initialize() { 
    ::simperf::default_initialize();
}

void test_default_asserts() {
  int x = 1;
  int y = 2;

#define TEST_ASSERT(check, left, right)                                                            \
  {                                                                                                \
    auto a = ::simperf::Assertion(check, left, right, _STRINGIZEX(check));                                               \
  if (a) {                                                                                         \
    SIMPERF_DEBUGBREAK();                                                                          \
  }}
  TEST_ASSERT(x == y, x, y);

#define TEST_ASSERT_WITH_TAGS(check, left, right, ...)                                             \
  {                                                                                                \
    auto a = ::simperf::Assertion(check, left, right,                                              \
                                  _STRINGIZEX(check),                          \
                                  _STRINGIZEX(left),       \
                                  _STRINGIZEX(right), ::simperf::make_array(__VA_ARGS__));         \
  if (a) {                                                                                        \
    SIMPERF_DEBUGBREAK();                                                                          \
  }}

  //::simperf::ctx::AddTag("simperf");
  //::simperf::ctx::AddTag("libtt");
  //::simperf::ctx::SetInstrumentTagStatus("libtt", false);
  //TEST_ASSERT_WITH_TAGS(x == y, x, y, "simperf", "libtt");
  //::simperf::ctx::SetInstrumentTagStatus("libtt", true);
  //TEST_ASSERT_WITH_TAGS(x == y, x, y, "simperf", "libtt");

#define TEST_ASSERT_WITH_TYPE(check, left, right, ...)                                             \
  {                                                                                                \
    auto a =                                                                                       \
        ::simperf::Assertion(check, left, right, _STRINGIZEX(check),                          \
                                  _STRINGIZEX(left),       \
                                  _STRINGIZEX(right),::simperf::make_array(__VA_ARGS__));        \
    if (a) {                                                                                      \
      SIMPERF_DEBUGBREAK();                                                                        \
    }                                                                                              \
  }

  //::simperf::ctx::SetAssertionTypeStatus(::simperf::AssertionType::Fatal, true);
  //TEST_ASSERT_WITH_TYPE(x == y, x, y);
}