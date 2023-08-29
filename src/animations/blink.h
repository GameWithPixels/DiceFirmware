#pragma once

#include "animations/animation_simple.h"

namespace Animations
{
    // Helper class to make all LEDs to blink some color
    struct Blink
    {
        void play(
            uint32_t color,
            uint16_t durationMs,
            uint8_t flashCount = 1,
            uint8_t fade = 0,
            uint32_t faceMask = ANIM_FACEMASK_ALL_LEDS,
            uint8_t loopCount = 1);

    private:
        AnimationSimple blinkAnim;
    };
}
