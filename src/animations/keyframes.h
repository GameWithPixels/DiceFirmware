#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
    /// <summary>
    /// Stores a single keyframe of an LED animation
    /// size: 2 bytes, split this way:
    /// - 9 bits: time 0 - 511 in 500th of a second (i.e )
    ///   + 1    -> 0.002s
    ///   + 500  -> 1s
    /// - 7 bits: color lookup (128 values)
    /// </summary>
    struct RGBKeyframe
    {
    public:
        uint16_t timeAndColor;

        uint16_t time() const; // unpack the time in ms
        uint32_t color(const DataSet::AnimationBits* bits) const;// unpack the color using the lookup table from the animation set

        void setTimeAndColorIndex(uint16_t timeMs, uint16_t colorIndex);
    };

    /// <summary>
    /// An animation track is essentially an animation curve and a set of LEDs.
    /// size: 8 bytes (+ the actual keyframe data).
    /// </summary>
    struct RGBTrack
    {
    public:
        uint16_t keyframesOffset;	// offset into a global keyframe buffer
        uint8_t keyFrameCount;		// Keyframe count
        uint8_t padding;
        uint32_t ledMask; 			// indicates which LEDs to drive

        // Tracks are expected to 1s long
        uint16_t getDuration(const DataSet::AnimationBits* bits) const;
        const RGBKeyframe& getRGBKeyframe(const DataSet::AnimationBits* bits, uint16_t keyframeIndex) const;
        int evaluate(const DataSet::AnimationBits* bits, int time, int retIndices[], uint32_t retColors[]) const;
        uint32_t evaluateColor(const DataSet::AnimationBits* bits, int time) const;
        int extractLEDIndices(int retIndices[]) const;
    };

    /// <summary>
    /// Stores a single keyframe of an LED animation
    /// size: 2 bytes, split this way:
    /// - 9 bits: time 0 - 511 in 500th of a second (i.e )
    ///   + 1    -> 0.002s
    ///   + 500  -> 1s
    /// - 7 bits: intensity (0 - 127)
    /// </summary>
    struct Keyframe
    {
        uint16_t timeAndIntensity;

        uint16_t time() const;
        uint8_t intensity() const;

        void setTimeAndIntensity(uint16_t timeMs, uint8_t intensity);
    };

    /// <summary>
    /// An animation track is essentially an animation curve and a set of LEDs.
    /// size: 8 bytes (+ the actual keyframe data).
    /// </summary>
    struct Track
    {
    public:
        uint16_t keyframesOffset;	// offset into a global keyframe buffer
        uint8_t keyFrameCount;		// Keyframe count
        uint8_t padding;
        uint32_t ledMask; 			// indicates which LEDs to drive

        // Tracks are expected to 1s long
        uint16_t getDuration(const DataSet::AnimationBits *bits) const;
        const Keyframe& getKeyframe(const DataSet::AnimationBits* bits, uint16_t keyframeIndex) const;
        int evaluate(const DataSet::AnimationBits* bits, uint32_t color, int time, int retIndices[], uint32_t retColors[]) const;
        uint32_t modulateColor(const DataSet::AnimationBits* bits, uint32_t color, int time) const;
        int extractLEDIndices(int retIndices[]) const;
    };


}

#pragma pack(pop)
