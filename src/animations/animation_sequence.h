#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

#define MAX_SEQ_ANIMATIONS 4

namespace Animations
{
    struct AnimationSequenceItem
    {
        uint16_t animationIndex;
        uint16_t animationDelay;
    };

    /// <summary>
    /// Animation that triggers other animations
    /// </summary>
    struct AnimationSequence
        : public Animation
    {
        AnimationSequenceItem animations[MAX_SEQ_ANIMATIONS];
        uint8_t animationCount;
    };

    /// <summary>
    /// Procedural on off animation instance data
    /// </summary>
    class AnimationInstanceSequence
        : public AnimationInstance
    {
    private:
        int lastMillis; // The last millis() value
    public:
        AnimationInstanceSequence(const AnimationSequence* preset, const DataSet::AnimationBits* bits);
        virtual ~AnimationInstanceSequence();
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int update(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);

    private:
        const AnimationSequence* getPreset() const;
        void processAnimations(int ms);
    };
}

#pragma pack(pop)