#include "attract_mode_controller.h"
#include "nrf_log.h"
#include "drivers_nrf/timers.h"
#include "modules/anim_controller.h"
#include "data_set/data_set.h"
#include "drivers_nrf/power_manager.h"
#include "animations/blink.h"

using namespace DriversNRF;
using namespace Modules;

#define ATTRACT_MODE_TIMER_MS 1000

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

        currentState = State_Attract;

        NRF_LOG_INFO("Attract Mode init");
    }

    void update(void* context) {
        switch (currentState) {
            case State_Off:
                // We are waiting for the die to be put on the charger
                switch (BatteryController::getBatteryState()) {
                    case BatteryController::BatteryState_Ok:
                    case BatteryController::BatteryState_Low:
                    case BatteryController::BatteryState_Error:
                    case BatteryController::BatteryState_BadCharging:
                        // Do nothing
                        break;
                    case BatteryController::BatteryState_Charging:
                    case BatteryController::BatteryState_Done:
                        currentState = State_Attract;
                        break;
                }
                break;
            case State_Recharging:
                // We are waiting for the recharge to be done
                switch (BatteryController::getBatteryState()) {
                    case BatteryController::BatteryState_Charging:
                    case BatteryController::BatteryState_BadCharging:
                        {
                        // Blink repeatedly
                        int millis = Timers::millis();
                        if (millis > nextAnimationStartMs) {
                            nextAnimationStartMs = millis + 3000;
                            static Blink blink;
                            auto layout = Config::DiceVariants::getLayout(Config::SettingsManager::getLayoutType());
                            blink.play(0x040000, 1000, 1, 255, layout->getTopFace(), 1);
                        }
                        }
                        break;
                    case BatteryController::BatteryState_Ok:
                    case BatteryController::BatteryState_Low:
                    case BatteryController::BatteryState_Error:
                        // Removed from the charger, so go back to off state
                        currentState = State_Off;
                        break;
                    case BatteryController::BatteryState_Done:
                        // Done charging, so go to attract mode
                        currentState = State_Attract;
                        break;
                }
                break;
            case State_Attract:
                // Do attract mode stuff unless we're removed from the charger
                switch (BatteryController::getBatteryState()) {
                    case BatteryController::BatteryState_BadCharging:
                    case BatteryController::BatteryState_Ok:
                    case BatteryController::BatteryState_Low:
                    case BatteryController::BatteryState_Error:
                        // Go to off mode
                        currentState = State_Off;
                        break;
                    case BatteryController::BatteryState_Charging:
                    case BatteryController::BatteryState_Done:
                        if (BatteryController::getLevelPercent() < 10) {
                            // Go to recharging mode
                            currentState = State_Recharging;
                        } else {
                            // Do attract mode stuff
                            int millis = Timers::millis();
                            if (millis > nextAnimationStartMs) {
                                auto anim = DataSet::getAnimation(nextAnimationIndex);
                                nextAnimationIndex = (nextAnimationIndex + 1) % DataSet::getAnimationCount();
                                nextAnimationStartMs = millis + anim->duration;
                                AnimController::play(anim, DataSet::getAnimationBits(), Accelerometer::currentFace());
                            }
                        }
                        break;
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
