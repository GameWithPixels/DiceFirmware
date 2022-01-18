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
	namespace KXTJ3
	{
		void init();
		void read(Core::float3* outAccel);

		void enableInterrupt();
        void enableDataInterrupt();
		void disableInterrupt();
		void clearInterrupt();

		void lowPower();

		// Notification management
		typedef void(*KXTJ3ClientMethod)(void* param, const Core::float3& acceleration);
		void hook(KXTJ3ClientMethod method, void* param);
		void unHook(KXTJ3ClientMethod client);
		void unHookWithParam(void* param);
	}
}

