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
	namespace LIS2DW12
	{
		void init();
		void read(Core::float3* outAccel);

		void enableInterrupt();
		void disableInterrupt();
		void clearInterrupt();

		void lowPower();
	}
}

