#include "die.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/log.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/behavior_controller.h"
#include "modules/charger_proximity.h"
#include "modules/temperature.h"
#include "modules/validation_manager.h"

using namespace Modules;
using namespace Bluetooth;
using namespace DriversNRF;
using namespace DataSet;

#define CHARGER_STATE_CHANGE_FADE_OUT_MS 250

namespace Die
{
    void onConnectionEvent(void* token, bool connected);
    void onChargerStateChange(void* param, ChargerProximity::ChargerProximityState newState);
    
    void initMainLogic() {
        Stack::hook(onConnectionEvent, nullptr);

        NRF_LOG_DEBUG("Main Logic init");
    }

    void initDieLogic() {
        ChargerProximity::hook(onChargerStateChange, nullptr);

        NRF_LOG_DEBUG("Die Logic init");
    }

    void onPowerEvent(PowerManager::PowerManagerEvent event) {
        switch (event) {
            case PowerManager::PowerManagerEvent_PrepareSysOff:
                //NRF_LOG_INFO("Going to system off mode");
                Accelerometer::stop();
                Accelerometer::lowPower();
                AnimController::stop();
                BatteryController::setControllerOverrideMode(BatteryControllerMode::ControllerOverrideMode_ForceEnableCharging);
                break;
            case PowerManager::PowerManagerEvent_PrepareWakeUp:
                //NRF_LOG_INFO("Going to low power mode");
                Accelerometer::stop();
                Accelerometer::lowPower();
                AnimController::stop();
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Slow);
                BatteryController::setControllerOverrideMode(BatteryControllerMode::ControllerOverrideMode_ForceEnableCharging);
                Temperature::slowMode(true);
                break;
            case PowerManager::PowerManagerEvent_PrepareSleep:
                //NRF_LOG_INFO("Going to Sleep");
                Accelerometer::stop();
                //AnimController::stop();
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Slow);
                BatteryController::setControllerOverrideMode(BatteryControllerMode::ControllerOverrideMode_ForceEnableCharging);
                Temperature::slowMode(true);
                Stack::stopAdvertising();

                if (ValidationManager::inValidation()) {
                    // In validation mode we just go to system off mode and rely
                    // on the magnet to power cycle the chip
                    PowerManager::goToSystemOff();
                } else {
                    // Set interrupt pin to wake up power manager
                    Accelerometer::enableInterrupt([](void* param) {
                        Scheduler::push(nullptr, 0, [](void* ignoreData, uint16_t ignoreSize) {
                            // Wake up
                            PowerManager::wakeFromSleep();
                        });
                    }, nullptr);
                }
                break;
            case PowerManager::PowerManagerEvent_WakingUpFromSleep:
                //NRF_LOG_INFO("Resuming from Sleep");
                Accelerometer::wakeUp();
                //AnimController::start();
                BatteryController::setControllerOverrideMode(BatteryControllerMode::ControllerOverrideMode_Default);
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Normal);
                Temperature::slowMode(false);
                Stack::startAdvertising();
                break;
            default:
                break;
        }

    }

    void onConnectionEvent(void* token, bool connected) {
        if (!connected) {
            PowerManager::resume();
        } else {
            PowerManager::pause();
        }
    }

    void onChargerStateChange(void* param, ChargerProximity::ChargerProximityState newState) {
        switch (newState) {
            case ChargerProximity::ChargerProximityState_Off:
                // Re-enable accelerometer animations
                BehaviorController::EnableAccelerometerRules();
                break;
            case ChargerProximity::ChargerProximityState_On:
                // Disable accelerometer-based animations
                BehaviorController::DisableAccelerometerRules();

                // Kill any currently executing accelerometer-triggered animation
                AnimController::fadeOutAnimsWithTag(
                    Animations::AnimationTag_Accelerometer,
                    CHARGER_STATE_CHANGE_FADE_OUT_MS);
                break;
        }
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
