#pragma once

#include <stdint.h>

#include "drivers_nrf/power_manager.h"

namespace Die
{
    void init();
    void update();

    void initMainLogic();
    void initDieLogic();
    void onPowerEvent(DriversNRF::PowerManager::PowerManagerEvent event);
}

