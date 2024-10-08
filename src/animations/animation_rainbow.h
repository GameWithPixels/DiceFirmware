#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Procedural rainbow animation data
    /// </summary>
    struct AnimationRainbow
        : public Animation
    {
        uint32_t faceMask;
        uint8_t count;
        uint8_t fade;
        uint8_t intensity;
        uint8_t cyclesTimes10;
    };

    /// <summary>
    /// Procedural rainbow animation instance data
    /// </summary>
    class AnimationInstanceRainbow
        : public AnimationInstance
    {
    public:
        AnimationInstanceRainbow(const AnimationRainbow* preset, const DataSet::AnimationBits* bits);
        virtual ~AnimationInstanceRainbow();
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int stop(int retIndices[]);
        virtual void updateDaisyChainLEDs(int ms, uint32_t* outDaisyChainColors);

    private:
        const AnimationRainbow* getPreset() const;
    };
}

#pragma pack(pop)
