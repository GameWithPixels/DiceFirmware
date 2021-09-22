#pragma once

#include <stdint.h>

namespace Modules
{
	/// <summary>
	/// The component in charge of controlling LEDs
	/// </summary>
	namespace LEDs
	{
        void init();
        void setPixelColor(uint16_t n, uint32_t c);
        void setPixelColors(int* indices, uint32_t* colors, int count);
        void setPixelColors(uint32_t* colors);
        void setAll(uint32_t c);
        void clear();

		typedef void(*LEDClientMethod)(void* param, bool powerOn);
		void hookPowerState(LEDClientMethod method, void* param);
		void unHookPowerState(LEDClientMethod client);
		void unHookPowerStateWithParam(void* param);
    }
}
