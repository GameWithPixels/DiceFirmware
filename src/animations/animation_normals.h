#pragma once

#include "animations/Animation.h"
#include "core/int3.h"

#pragma pack(push, 1)

namespace Animations
{
    enum NormalsColorOverrideType : uint8_t
    {
        NormalsColorOverrideType_None = 0,
        NormalsColorOverrideType_FaceToGradient,
        NormalsColorOverrideType_FaceToRainbowWheel,
    };

    /// <summary>
    /// Procedural gradient animation based on normals
    /// </summary>
    struct AnimationNormals
        : public Animation
    {
        uint16_t gradientOverTime; // 0 - 1, over time
        uint16_t gradientAlongAxis; // 0 = top, 1 = bottom
        uint16_t gradientAlongAngle; // 0 = -pi, 1 = pi
        int16_t axisScaleTimes1000;
        int16_t axisOffsetTimes1000;
        int16_t axisScrollSpeedTimes1000; // in cycles, can be negative
        int16_t angleScrollSpeedTimes1000; // in cycles, can be negative
        uint8_t fade; // 0 - 255
        NormalsColorOverrideType mainGradientColorType;
        uint16_t mainGradientColorVar; // 0 - 1000, only applies for random and face-based color
    };

    /// <summary>
    /// Procedural rainbow animation instance data
    /// </summary>
    class AnimationInstanceNormals
        : public AnimationInstance
    {
    public:
        AnimationInstanceNormals(const AnimationNormals* preset, const DataSet::AnimationBits* bits);
        virtual ~AnimationInstanceNormals();
        virtual int animationSize() const;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int update(int ms, int retIndices[], uint32_t retColors[]);
        virtual int stop(int retIndices[]);

    private:
        const AnimationNormals* getPreset() const;
        const Core::int3* normals;
        const Core::int3* faceNormal;
        Core::int3 backVector;
        int baseColorParam;
    };
}

#pragma pack(pop)