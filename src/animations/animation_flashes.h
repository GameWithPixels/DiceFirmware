#pragma once

#include "animations/animation.h"
#include "animations/animation_parameters.h"

#pragma pack(push, 1)

namespace Animations
{
    enum AnimationFlashesFlags : uint8_t
    {
        AnimationFlashesFlags_None		  = 0,
        AnimationFlashesFlags_CaptureColor  = 1 << 0,
    };

    /// <summary>
    /// Procedural on off animation
    /// </summary>
    struct AnimationFlashes
        : public Animation
    {
        DColorVectorPtr colors;
    };

    /// <summary>
    /// Procedural on off animation instance data
    /// </summary>
    struct AnimationFlashesInstance
        : public AnimationInstance
    {
    private:
        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);

    private:
        const AnimationFlashes* getPreset() const;

        uint32_t capturedColor;
    };
}

#pragma pack(pop)
