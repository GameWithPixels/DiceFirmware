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
#include "modules/behavior_controller.h"

using namespace Modules;
using namespace Bluetooth;
using namespace DriversNRF;

namespace Die
{
    static UserMode currentUserMode = UserMode_Default;

    UserMode getCurrentUserMode() {
        return currentUserMode;
    }

    void beginRemoteControlledMode() {
        if (currentUserMode != UserMode_RemoteControlled) {
            // Turn off behavior controller
            BehaviorController::DisableAccelerometerRules();

            // Enter remote controlled mode
            currentUserMode = UserMode_RemoteControlled;
            NRF_LOG_INFO("Begin Remote Controlled Mode");
        }
    }

    void exitRemoteControlledMode() {
        if (currentUserMode == UserMode_RemoteControlled) {
            // Turn accelerometer rules back on
            BehaviorController::EnableAccelerometerRules();

            // Do stuff and then exit remote controlled mode
            currentUserMode = UserMode_Default;
            NRF_LOG_INFO("Exitted Remote Controlled Mode");
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
