#pragma once

#include "animations/animation_simple.h"
#include "data_set/data_animation_bits.h"

namespace Animations
{
    // Helper class to make all LEDs to blink some color
    struct Blink
    {
        Blink();
        void play(
            uint32_t color,
            uint16_t durationMs,
            uint8_t flashCount = 1,
            uint8_t fade = 0,
            uint32_t faceMask = ANIM_FACEMASK_ALL_LEDS,
            uint8_t loopCount = 1);

    private:
        AnimationSimple blinkAnim;
        uint8_t animPalette[3];
        DataSet::AnimationBits animBits;
    };
}
