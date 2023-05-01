/*
 * NeoPixel WS2812B Driver for nRF52 series.
 * Custom bit-bang version for NRF52810
 */

#ifndef NRF_NEOPIXEL_BITBANG_H__
#define NRF_NEOPIXEL_BITBANG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void NeopixelBitBangSetLEDs(uint32_t* colors, int count, uint8_t dataPin, uint8_t powerPin);
void NeopixelBitBangSetD20Face20LED(uint32_t color);
#ifdef __cplusplus
}
#endif

#endif //NRF_NEOPIXEL_BITBANG_H__