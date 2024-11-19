#include "attract_mode_controller.h"
#include "nrf_log.h"
#include "drivers_nrf/timers.h"
#include "modules/anim_controller.h"
#include "data_set/data_set.h"
#include "drivers_nrf/power_manager.h"

using namespace DriversNRF;
using namespace Modules;

#define ATTRACT_MODE_TIMER_MS 1000

namespace Modules::AttractModeController
{
    APP_TIMER_DEF(attractModeTimer);

    int nextAnimationStartMs;
    int nextAnimationIndex;

    void update(void* context);

    void init() {

        Timers::createTimer(&attractModeTimer, APP_TIMER_MODE_REPEATED, update);
        Timers::startTimer(attractModeTimer, ATTRACT_MODE_TIMER_MS, NULL);

        auto anim = DataSet::getAnimation(0);
        nextAnimationIndex = 1 % DataSet::getAnimationCount();
        nextAnimationStartMs = Timers::millis() + anim->duration;
        AnimController::play(anim, DataSet::getAnimationBits(), Accelerometer::currentFace());

        NRF_LOG_INFO("Attract Mode init");
    }

    void update(void* context) {
        PowerManager::feed();
        int millis = Timers::millis();
        if (millis > nextAnimationStartMs) {
            auto anim = DataSet::getAnimation(nextAnimationIndex);
            nextAnimationIndex = (nextAnimationIndex + 1) % DataSet::getAnimationCount();
            nextAnimationStartMs = millis + anim->duration;
            AnimController::play(anim, DataSet::getAnimationBits(), Accelerometer::currentFace());
        }
    }
}
