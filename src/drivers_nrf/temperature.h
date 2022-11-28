#pragma once

namespace DriversNRF
{
    namespace Temperature
    {
        typedef void (*TemperatureInitCallback)(bool success);

        void init(TemperatureInitCallback callback);

		// Notification management
		typedef void(*TemperatureClientMethod)(void* param, int temperatureTimes100);
		void hook(TemperatureClientMethod method, void* param);
		void unHook(TemperatureClientMethod client);
		void unHookWithParam(void* param);
    }
}
