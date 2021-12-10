#pragma once

#include <stdint.h>

namespace Core
{
    struct float3;
}

namespace DriversHW
{
	/// <summary>
	/// The accelerometer I2C devices
	/// </summary>
	namespace LIS2DE12
	{
		void init();
		void read(Core::float3* outAccel);

		void enableInterrupt();
        void enableDataInterrupt();
		void disableInterrupt();
		void clearInterrupt();

		void lowPower();

		// Notification management
		typedef void(*LIS2DE12ClientMethod)(void* param, const Core::float3& acceleration);
		void hook(LIS2DE12ClientMethod method, void* param);
		void unHook(LIS2DE12ClientMethod client);
		void unHookWithParam(void* param);
	}
}

