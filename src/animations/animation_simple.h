#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Procedural on off animation
    /// </summary>
    struct AnimationSimple
        : public Animation
    {
        uint32_t faceMask;
        uint16_t colorIndex;
        uint8_t count;
        uint8_t fade;
    };

    /// <summary>
    /// Procedural on off animation instance data
    /// </summary>
    class AnimationInstanceSimple
        : public AnimationInstance
    {
    private:
        uint32_t rgb; // The color is determined at the beginning of the animation
    public:
        AnimationInstanceSimple(const AnimationSimple* preset, const DataSet::AnimationBits* bits);
        virtual ~AnimationInstanceSimple();
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int update(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);

    private:
        const AnimationSimple* getPreset() const;
    };
}

#pragma pack(pop)