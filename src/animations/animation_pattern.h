#pragma once

#include "animations/animation.h"
#include "animations/ledpattern.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Procedural pattern animation data
    /// </summary>
    // Animation implementation that uses an LED Design and color / intensity overrides
    // This should be the most common animation
    struct AnimationPattern
        : public Animation
    {
        // The design is what indicates when the LEDs should turn on or off
        Profile::Pointer<LEDPattern> pattern;

        // Then the color and intensity curves define the specifics of how the LEDs turn on
        DColorPtr colorOverTime;     // Works best if one of these two colors is white
        DColorPtr colorOverBlink;    // and the other is the actual curve, but you can use both.
                                        // The colors will be multiplied together
        DScalarPtr intensityOverTime;    // Works best if one of these two curves is constant 1 (or 255)
        DScalarPtr intensityOverBlink;   // the two curves will be multiplied together
   };
    // size: 11 bytes + design

    // Instance data for a pattern-based animation
    struct AnimationPatternInstance
        : public AnimationInstance
    {
        LEDPatternInstance* patternInstance;
        // Pattern Instance data

        virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
        virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);
    };
}

#pragma pack(pop)
