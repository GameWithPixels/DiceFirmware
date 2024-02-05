#pragma once

#include "animations/Animation.h"
#include "config/settings.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Procedural rainbow animation that cycle faces
    /// </summary>
    struct AnimationWorm
        : public Animation
    {
        uint32_t faceMask;
        uint8_t count;
        uint8_t fade;
        uint8_t intensity;
        uint8_t cyclesTimes10;
        uint16_t gradientTrackOffset;
    };

    /// <summary>
    /// Procedural rainbow animation instance data
    /// </summary>
    class AnimationInstanceWorm
        : public AnimationInstance
    {
    public:
        AnimationInstanceWorm(const AnimationWorm *preset, const DataSet::AnimationBits *bits);
        virtual ~AnimationInstanceWorm();
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);

    private:
        const AnimationWorm *getPreset() const;
        uint8_t indices[MAX_COUNT];
    };
}

#pragma pack(pop)