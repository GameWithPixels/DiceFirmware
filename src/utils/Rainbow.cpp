// 
// 
// 

#include "rainbow.h"
#include "utils.h"
#include "nrf_delay.h"

#define NUMPIXELS 21

namespace Rainbow
{
	// Input a value 0 to 255 to get a color value.
	// The colours are a transition r - g - b - back to r.
	uint32_t wheel(uint8_t WheelPos, uint8_t intensity)
	{
		if (WheelPos < 85)
		{
			return Utils::toColor(WheelPos * 3 * intensity / 255, (255 - WheelPos * 3) * intensity / 255, 0);
		}
		else if (WheelPos < 170)
		{
			WheelPos -= 85;
			return Utils::toColor((255 - WheelPos * 3) * intensity / 255, 0, WheelPos * 3 * intensity / 255);
		}
		else
		{
			WheelPos -= 170;
			return Utils::toColor(0, WheelPos * 3 * intensity / 255, (255 - WheelPos * 3) * intensity / 255);
		}
	}

	uint32_t faceWheel(uint8_t face, uint8_t count) {
		return wheel((face * 256) / count);
	}


	constexpr uint8_t paletteBits = 6;
	constexpr uint8_t paletteSize = 1 << paletteBits;

	uint32_t palette(uint8_t index) {
		uint32_t color = 0;
		uint8_t intensity = 0;
		uint8_t whiteness = 0;
		uint8_t wheel = 0;
		if (index < paletteSize / 2) {
			// Full saturation of the colors
			whiteness = 0;
			wheel = index << (8 - paletteBits);
		} else {
			// Half saturation
			index -= paletteSize / 2;
			if (index < paletteSize / 4) {
				whiteness = 127;
				wheel = index << (9 - paletteBits);
			} else {
				// Quarter saturation
				index -= paletteSize / 4;
				if (index < paletteSize / 8) {
					whiteness = 127+64;
					wheel = index << (10 - paletteBits);
				} else {
					// everything else is white
					return 0xFFFFFF;
				}
			}
		}

		// We want the color to be always about the same intensity
		intensity = 255 - whiteness;
		// NOTE: On computer/phone, we want to double this value to get nicely saturated colors for the in-between colors (yellow, cyan, purple)
		// On the LEDs we don't because we have a separate led emitter for each r, g and b. If we did then the yellows and cyans would appear
		// brighter than the monochromatic colors (red, green blue).
		// intensity *= 2; //<-- Uncomment on computer/phone

		uint8_t r = whiteness;
		uint8_t g = whiteness;
		uint8_t b = whiteness;
		if (wheel < 85) {
			// First third of the rainbow
			r += wheel * 3 * intensity / 255;
			g += (255 - wheel * 3) * intensity / 255;
		} else if (wheel < 170) {
			// Second third of the rainbow
			wheel -= 85;
			r += (255 - wheel * 3) * intensity / 255;
			b += wheel * 3 * intensity / 255;
		} else {
			// Last third of the rainbow
			wheel -= 170;
			g += wheel * 3 * intensity / 255;
			b += (255 - wheel * 3) * intensity / 255;
		}
		return Utils::toColor(r, g, b);
	}
}

