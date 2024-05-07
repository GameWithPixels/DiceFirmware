#pragma once

#include "animations/animation.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Procedural rainbow animation data
    /// </summary>
    struct AnimationRainbow
        : public Animation
    {
        uint8_t count;
        uint8_t fade;
        uint8_t intensity;
        uint8_t cyclesTimes16; // How to stretch or repeat the rainbow across the LEDs
        bool traveling; // If true, the rainbow will move across the LEDs
    };

    /// <summary>
    /// Procedural rainbow animation instance data
    /// </summary>
    struct AnimationRainbowInstance
        : public AnimationInstance
    {
        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);

    private:
        const AnimationRainbow* getPreset() const {
            return static_cast<const AnimationRainbow*>(animationPreset);
        }
    };
}

#pragma pack(pop)
