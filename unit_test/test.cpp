#if UNIT_TEST

#include "nrf_log.h"
#include "src\drivers_nrf\scheduler.h"

using namespace DriversNRF;

void test1()
{
    NRF_LOG_ERROR("Running Unit Test 1");
}

void test2()
{
    NRF_LOG_ERROR("Running Unit Test 2");
}

void runNextTest(void *p_event_data, uint16_t event_size)
{
    typedef void (*TestFunc)();
    static TestFunc testFunctions[] = {&test1, &test2};
    static uint32_t testCounter = 0;

    testFunctions[testCounter]();
    ++testCounter;

    if (testCounter < sizeof(testFunctions) / sizeof(TestFunc))
    {
        Scheduler::push(nullptr, 0, runNextTest);
    }
}

void startUnitTesting()
{
    NRF_LOG_ERROR("Starting Unit Tests!");
    Scheduler::push(nullptr, 0, runNextTest);
}

#endif
