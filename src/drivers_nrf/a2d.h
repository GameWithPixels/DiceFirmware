#pragma once

#include <stdint.h>
#include <stddef.h>

namespace DriversNRF
{
	/// <summary>
	/// Wrapper for the Wire library that is set to use the Die pins
	/// </summary>
	namespace A2D
	{
		void init();

        float readVBat();
        float read5V();
        float readVLED();
        float readVBoard();
        float readVDD();
        float readVNTC();

		void selfTest();
		void selfTestBatt();
	}
}

