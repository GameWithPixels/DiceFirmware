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
        void goToSleep();
        void wakeFromSleep();
        void reset();

        enum PowerManagerEvent
        {
            PowerManagerEvent_PrepareSysOff,
            PowerManagerEvent_PrepareWakeUp,
            PowerManagerEvent_PrepareDFU,
            PowerManagerEvent_PrepareReset,
            PowerManagerEvent_PrepareSleep,
            PowerManagerEvent_WakingUpFromSleep,
        };

		typedef void(*PowerManagerClientMethod)(PowerManagerEvent event);
		void setPowerEventCallback(PowerManagerClientMethod method);
		void clearPowerEventCallback();
        bool checkFromSysOff();
    }
}
