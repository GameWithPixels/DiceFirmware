#if UNIT_TEST

#include "nrf_log.h"
#include "../src/die.h"
#include "src\drivers_nrf\scheduler.h"
#include "../data_set/data_set.h"
#include "../modules/anim_controller.h"
#include "System.h"
#include "uCUnit-v1.0.h"
#include "../src/drivers_nrf/flash.h"
#include "../src/animations/Animation.h"

using namespace DriversNRF;
using namespace Modules;
using namespace Behaviors;

void test1()
{
    UCUNIT_TestcaseBegin("Dataset Tests");

    /* Test correct animation count for default dataset */
    UCUNIT_CheckIsEqual(6, DataSet::getAnimationCount());
    
    /* Test valid and invalid edge parameters for animation dataset functions */ 
    UCUNIT_CheckIsNull(DataSet::getAnimation(DataSet::getAnimationCount()));
    UCUNIT_CheckIsNull(DataSet::getAnimation(-1));
    UCUNIT_CheckIsNotNull(DataSet::getAnimation(DataSet::getAnimationCount()-1));
    UCUNIT_CheckIsNotNull(DataSet::getAnimation(0));

    /* Test valid and invalid edge parameters for condition dataset functions */ 
    UCUNIT_CheckIsNull(DataSet::getCondition(DataSet::getConditionCount()));
    UCUNIT_CheckIsNull(DataSet::getCondition(-1));
    UCUNIT_CheckIsNotNull(DataSet::getCondition(DataSet::getConditionCount()-1));
    UCUNIT_CheckIsNotNull(DataSet::getCondition(0));

    /* Test valid and invalid edge parameters for action dataset functions */ 
    UCUNIT_CheckIsNull(DataSet::getAction(DataSet::getActionCount()));
    UCUNIT_CheckIsNull(DataSet::getAction(-1));
    UCUNIT_CheckIsNotNull(DataSet::getAction(DataSet::getActionCount()-1));
    UCUNIT_CheckIsNotNull(DataSet::getAction(0));

    /* Test valid and invalid edge parameters for rule dataset functions */ 
    UCUNIT_CheckIsNull(DataSet::getRule(DataSet::getRuleCount()));
    UCUNIT_CheckIsNull(DataSet::getRule(-1));
    UCUNIT_CheckIsNotNull(DataSet::getRule(DataSet::getRuleCount()-1));
    UCUNIT_CheckIsNotNull(DataSet::getRule(0));

    /* Erase dataset, check for invalidity, rewrite dataset **not working yet** */
    // May not be possible
    /* erase function is async event, need callback to complete/end test */
    // Maybe need timeout/wait
    //Flash::erase(nullptr, Flash::getDataSetDataAddress(), Flash::bytesToPages(DataSet::dataSize()), NULL);

    UCUNIT_TestcaseEnd();
}

void test2()
{
    UCUNIT_TestcaseBegin("Animation Tests");


    /* Send too many animations to animation controller and check for crash */
    /* How to check and recover correctly? */
    //while (1) AnimController::play(1, Accelerometer::currentFace(), false);
    UCUNIT_TestcaseEnd();
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
    else UCUNIT_WriteSummary();
}

void startUnitTesting()
{
    NRF_LOG_ERROR("Starting Unit Tests!");
    Scheduler::push(nullptr, 0, runNextTest);
}

#endif
