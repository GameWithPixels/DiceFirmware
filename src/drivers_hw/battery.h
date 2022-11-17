#pragma once

namespace DriversHW
{
    namespace Battery
    {
        void init();
        float checkVBat();
        float checkVCoil();
        float checkVLED();
        bool checkCharging();

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

