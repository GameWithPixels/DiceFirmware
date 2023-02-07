#pragma once

#include "drivers_nrf/power_manager.h"

namespace Die
{
	void initMainLogic();
    void initDieLogic();
    void onPowerEvent(DriversNRF::PowerManager::PowerManagerEvent event);
}
