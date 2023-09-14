#include "die.h"
#include "die_private.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/log.h"
#include "bluetooth/bluetooth_message_service.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "notifications/battery.h"
#include "notifications/roll.h"
#include "notifications/rssi.h"

using namespace Modules;
using namespace Bluetooth;
using namespace DriversNRF;

namespace Die
{
    static TopLevelState currentTopLevelState = TopLevel_SoloPlay;

    TopLevelState getCurrentState() {
        return currentTopLevelState;
    }

    void enterStandardState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_Animator:
                // Animator mode had turned accelerometer off, restart it now
                Accelerometer::start();
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_Testing:
                // Testing mode had anim controller off, restart it now
                AnimController::start();
                Accelerometer::start();
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_SoloPlay:
                // Nothing to do
                break;
       }
    }

	void enterLEDAnimState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_SoloPlay:
                Accelerometer::stop();
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_Testing:
                AnimController::start();
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_Animator:
                // Nothing to do
                break;
       }
    }

    void enterTestingState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            case TopLevel_SoloPlay:
                Accelerometer::stop();
                AnimController::stop();
                currentTopLevelState = TopLevel_Testing;
                break;
            case TopLevel_Animator:
                AnimController::stop();
                currentTopLevelState = TopLevel_Testing;
                break;
            default:
                currentTopLevelState = TopLevel_Testing;
                break;
            case TopLevel_Testing:
                break;
       }
    }

    void initMainLogic() {
        initHandlers();

        Notifications::Battery::init();
        Notifications::Roll::init();
        Notifications::Rssi::init();

        NRF_LOG_DEBUG("Main Logic init");
    }

    void initDieLogic() {
        initLogicHandlers();

        NRF_LOG_DEBUG("Die Logic init");
    }

    // Main loop!
    void update() {

        Scheduler::update();
        Watchdog::feed();
        MessageService::update();
        if (!MessageService::needUpdate()) {
            // Skip calling power manager so the CPU doesn't go to sleep
            PowerManager::update();
        }
    }
}

int main() {
    Die::init();
    for (;;) {
        Die::update();

        #if NRF_LOG_DEFERRED
            Log::process();
        #endif
    }
    return 0;
}
