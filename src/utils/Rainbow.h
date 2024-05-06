#pragma once

#include <stdint.h>

namespace Rainbow
{
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t wheel(uint8_t wheelPos);
    uint32_t palette(uint8_t index);

    uint32_t faceWheel(uint8_t face, uint8_t count);
}
