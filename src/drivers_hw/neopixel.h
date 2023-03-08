/*
 * NeoPixel WS2812B Driver for nRF52 series.
 *  (c) 2019, Aquamarine Networks.
 */

#pragma once

#include <stdint.h>

namespace DriversHW
{
  namespace NeoPixel
  {
        void init();
        void uninit(void);
        void clear();
        void show(uint32_t* colors);
        void testLEDReturn();
    }
}
