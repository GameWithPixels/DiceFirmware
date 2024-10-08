#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
    struct RGBKeyframe;
    struct RGBTrack;

    /// <summary>
    /// A keyframe-based animation
    /// size: 8 bytes (+ actual track and keyframe data)
    /// </summary>
    struct AnimationKeyframed
        : public Animation
    {
        uint16_t tracksOffset; // offset into a global buffer of tracks
        uint16_t trackCount;
    };

    /// <summary>
    /// Keyframe-based animation instance data
    /// </summary>
    class AnimationInstanceKeyframed
        : public AnimationInstance
    {
    public:
        AnimationInstanceKeyframed(const AnimationKeyframed* preset, const DataSet::AnimationBits* bits);
        virtual ~AnimationInstanceKeyframed();

    public:
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int update(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);

    private:
        const AnimationKeyframed* getPreset() const;
        const RGBTrack& GetTrack(int index) const;
    };

}

#pragma pack(pop)