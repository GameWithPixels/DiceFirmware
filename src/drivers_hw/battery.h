#pragma once

#include "stdint.h"

namespace DriversHW
{
    namespace Battery
    {
        bool init();
        int32_t checkVBatTimes1000();
        int32_t checkVCoilTimes1000();
        bool checkCharging();
        void setDisableChargingOverride(bool disable);
        bool getDisableChargingOverride();

        enum ChargingEvent
        {
            ChargingEvent_ChargeStart = 0,
            ChargingEvent_ChargeStop,
        };

        typedef void(*ClientMethod)(void* param, ChargingEvent event);

        // Notification management
        void hook(ClientMethod method, void* param);
        void unHook(ClientMethod client);
        void unHookWithParam(void* param);

        void selfTest();
    }
}

