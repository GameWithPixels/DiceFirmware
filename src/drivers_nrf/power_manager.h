#pragma once

#include "nrf_pwr_mgmt.h"

namespace DriversNRF
{
    // Initializes the sdk log system
    namespace PowerManager
    {
        void init();
        void feed();
        void update();
        void pause();
        void resume();
        void goToSystemOff();
        void reset();

		typedef void(*PowerManagerClientMethod)(void* param, nrf_pwr_mgmt_evt_t event);
		void hook(PowerManagerClientMethod method, void* param);
		void unHook(PowerManagerClientMethod client);
    }
}
