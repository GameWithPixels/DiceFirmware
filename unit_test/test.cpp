#define NRF_LOG_ENABLED 1
#include "nrf_log.h"
#include "src\die.h"

void runUnitTest()
{
// UNIT_TEST should be defined = 1
#if !UNIT_TEST
    Oh no...
#endif
    Die::init();
    NRF_LOG_ERROR("Logs should work");
}