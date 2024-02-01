#pragma once

#include "animations/keyframes.h"

using namespace Animations;

namespace DataSet
{
    struct AnimationBits
    {
        // The palette for all animations, stored in RGB RGB RGB etc...
        // Maximum 128 * 3 = 376 bytes
        const uint8_t* palette;
        uint32_t paletteSize; // In bytes (divide by 3 for colors)

        #define PALETTE_COLOR_FROM_FACE     127
        #define PALETTE_COLOR_FROM_RANDOM   126

        // The individual RGB keyframes we have, i.e. time and color, packed in
        const Animations::RGBKeyframe* rgbKeyframes; // pointer to the array of tracks
        uint32_t rgbKeyFrameCount;

        // The RGB tracks we have
        const Animations::RGBTrack* rgbTracks; // pointer to the array of tracks
        uint32_t rgbTrackCount;

        // The individual intensity keyframes we have, i.e. time and intensity, packed in
        const Animations::Keyframe* keyframes; // pointer to the array of tracks
        uint32_t keyFrameCount;

        // The RGB tracks we have
        const Animations::Track* tracks; // pointer to the array of tracks
        uint32_t trackCount;

        // The animations. Because animations can be one of multiple classes (simple inheritance system)
        // The dataset stores an offset into the animations buffer for each entry. The first member of
        // The animation base class is a type enum indicating what it actually is.
		const uint16_t* animationOffsets; // offsets to actual animation from the animation pointer below
		uint32_t animationCount;

		const uint8_t* animations; // The animations we have, 4-byte aligned, so may need some padding
		uint32_t animationsSize; // In bytes

        // Palette
        uint16_t getPaletteSize() const;
        uint32_t getPaletteColor(uint16_t colorIndex) const;

        // Animation keyframes (time and color)
        const Animations::RGBKeyframe& getRGBKeyframe(uint16_t keyFrameIndex) const;
        uint16_t getRGBKeyframeCount() const;

        // RGB tracks, list of keyframes
        const Animations::RGBTrack& getRGBTrack(uint16_t trackIndex) const;
        Animations::RGBTrack const * const getRGBTracks(uint16_t tracksStartIndex) const;
        uint16_t getRGBTrackCount() const;

        // Animation keyframes (time and intensity)
        const Animations::Keyframe& getKeyframe(uint16_t keyFrameIndex) const;
        uint16_t getKeyframeCount() const;

        // RGB tracks, list of keyframes
        const Animations::Track& getTrack(uint16_t trackIndex) const;
        Animations::Track const * const getTracks(uint16_t tracksStartIndex) const;
        uint16_t getTrackCount() const;

        const Animation* getAnimation(int animationIndex) const;
        uint16_t getAnimationCount() const;

        void Clear();
    };
}