#pragma once

#include <stdint.h>
#include <stddef.h>

namespace DriversNRF
{
	/// <summary>
	/// Wrapper for the Random Number Generator
	/// </summary>
	namespace RNG
	{
		void init();
        uint8_t randomVectorGenerate(uint8_t * p_buff, uint8_t size);
        uint8_t randomUInt8();
        uint16_t randomUInt16();
        uint32_t randomUInt32();
	}
}

