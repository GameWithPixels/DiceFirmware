#include "attract_mode_controller.h"
#include "nrf_log.h"
#include "drivers_nrf/timers.h"
#include "modules/anim_controller.h"
#include "data_set/data_set.h"
#include "drivers_nrf/power_manager.h"
#include "animations/blink.h"
#include "modules/battery_controller.h"
#include "drivers_nrf/rng.h"

using namespace DriversNRF;
using namespace Modules;

#define ATTRACT_MODE_TIMER_MS 1000
#define MIN_BATTERY_LEVEL_PERCENT 20
#define MAX_BATTERY_LEVEL_PERCENT 50
#define VCOIL_OFF_THRESHOLD 3000 // milliVolts
#define VCOIL_ON_THRESHOLD 4000 // milliVolts

namespace Modules::AttractModeController
{
    APP_TIMER_DEF(attractModeTimer);

    int nextAnimationStartMs;
    int nextAnimationIndex;

    enum CurrentState {
        State_Unknown = 0,
        State_Off,
        State_Recharging,
        State_Attract,
        State_AttractCooldown,
    };

    CurrentState currentState = State_Unknown;

    void update(void* context);

    void init() {

        Timers::createTimer(&attractModeTimer, APP_TIMER_MODE_REPEATED, update);
        Timers::startTimer(attractModeTimer, ATTRACT_MODE_TIMER_MS, NULL);

        auto anim = DataSet::getAnimation(0);
        nextAnimationIndex = 1 % DataSet::getAnimationCount();
        nextAnimationStartMs = Timers::millis() + anim->duration;
        AnimController::play(anim, DataSet::getAnimationBits(), Accelerometer::currentFace());
        BatteryController::setControllerOverrideMode(BatteryController::ControllerOverrideMode_ForceEnableCharging);
        currentState = State_Attract;

        NRF_LOG_INFO("Attract Mode init");
    }

    void update(void* context) {
        auto coil = BatteryController::getCoilVoltageMilli();
        auto battery = BatteryController::getLevelPercent();
        switch (currentState) {
            case State_Off:
                // We are waiting for the die to be put on the charger
                if (coil > VCOIL_ON_THRESHOLD) {
                    if (battery > MIN_BATTERY_LEVEL_PERCENT) {
                        currentState = State_Attract;
                    } else {
                        currentState = State_Recharging;
                    }
                }
                // Else stay here
                break;
            case State_Recharging:
                if (coil > VCOIL_OFF_THRESHOLD) {
                    if (battery > MAX_BATTERY_LEVEL_PERCENT) {
                        currentState = State_Attract;
                    } else {
                        // Else stay here
                        // Blink repeatedly
                        int millis = Timers::millis();
                        if (millis > nextAnimationStartMs) {
                            nextAnimationStartMs = millis + 3000;
                            static Blink blink;
                            auto layout = Config::DiceVariants::getLayout(Config::SettingsManager::getLayoutType());
                            blink.play(0x040000, 1000, 1, 255, layout->getTopFace(), 1);
                        }
                    }
                } else {
                    // Came off the coil
                    currentState = State_Off;
                }
                break;
            case State_Attract:
                if (coil > VCOIL_OFF_THRESHOLD) {
                    if (battery > MIN_BATTERY_LEVEL_PERCENT) {
                        // Play animations
                        int millis = Timers::millis();
                        if (millis > nextAnimationStartMs) {
                            auto anim = DataSet::getAnimation(nextAnimationIndex);
                            nextAnimationIndex = RNG::randomUInt32() % DataSet::getAnimationCount();
                            nextAnimationStartMs = millis + anim->duration;
                            AnimController::play(anim, DataSet::getAnimationBits(), Accelerometer::currentFace());
                        }
                    } else {
                        // Go to recharging mode
                        currentState = State_Recharging;
                    }
                } else {
                    // Came off the coil
                    currentState = State_Off;
                }
                break;
            case State_AttractCooldown:
            default:
                NRF_LOG_WARNING("Attract Mode in invalid state");
                break;
        }
        
        PowerManager::feed();
    }
}
