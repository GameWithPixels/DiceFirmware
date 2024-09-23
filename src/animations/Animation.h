#pragma once

#include <stdint.h>
#include "animation_tag.h"

#pragma pack(push, 1)

namespace DataSet
{
    struct AnimationBits;
}

#define ANIM_FACEMASK_ALL_LEDS 0xFFFFFFFF

namespace Animations
{
    /// <summary>
    /// Defines the types of Animation Presets we have/support
    /// </summary>
    enum AnimationType : uint8_t
    {
        Animation_Unknown = 0,
        Animation_Simple,
        Animation_Rainbow,
        Animation_Keyframed,
        Animation_GradientPattern,
        Animation_Gradient,
        Animation_Noise,
        Animation_Cycle,
        Animation_BlinkId,
        Animation_Normals,
        Animation_Sequence,
        Animation_Worm,
    };

    /// <summary>
    /// Flags for the animations, they can be combined.
    /// </summary>
    enum AnimationFlags : uint8_t
    {
        AnimationFlags_None,
        AnimationFlags_Traveling = 1,     // Make the animation travel around the dice, only available for the Rainbow animation
    };

    /// <summary>
    /// Defines the type of index returned by the animation
    /// </summary>
    enum AnimationIndexType : uint8_t
    {
        AnimationIndexType_Face       = 0,    // Animation indices refer to face indices
        AnimationIndexType_DaisyChain = 1,    // Animation indices refer to daisy chain indices, which are the indices of the LEDs in the daisy chain
        AnimationIndexType_Led        = 2,    // Animation indices refer to led indices, which for most dice (except PD6) are the same as face indices
    };

    /// <summary>
    /// Base struct for animation presets. All presets have a few properties in common.
    /// Presets are stored in flash, so do not have methods or vtables or anything like that.
    /// </summary>
    struct Animation
    {
        AnimationType type;
        uint8_t animFlags; // Combination of AnimationFlags
        uint16_t duration; // in ms

        bool isTraveling() const {
            return (animFlags & AnimationFlags_Traveling) != 0;
        }
        void setTraveling(bool value) {
            if (value) {
                animFlags |= AnimationFlags_Traveling;
            } else {
                animFlags &= ~AnimationFlags_Traveling;
            }
        }
        AnimationIndexType getIndexType() const {
            return (AnimationIndexType)((animFlags >> 1) & 0b11);
        }
        void setIndexType(AnimationIndexType value) {
            animFlags = (animFlags & 0b11111001) | (value << 1);
        }
    };

    /// <summary>
    /// Animation instance data, refers to an animation preset but stores the instance data and
    /// (derived classes) implements logic for displaying the animation.
    /// </summary>
    class AnimationInstance
    {
    public:
        Animation const * animationPreset;
        const DataSet::AnimationBits* animationBits;
        int startTime; //ms
        int forceFadeTime; //ms, used when fading out (because anim is being replaced), -1 otherwise
        AnimationTag tag; // used to identify where the animation came from / what system triggered it
        uint8_t remapFace;
        uint8_t loopCount;
        uint8_t paddingLoopCount;

    protected:
        AnimationInstance(const Animation* preset, const DataSet::AnimationBits* bits);

    public:
        virtual ~AnimationInstance();
        // starts the animation, with the option of repeating it if _loopCount > 1
        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int animationSize() const = 0;
        // method used to set which faces to turn on as well as the color of their LEDs
        // retIndices is one to one with retColors and keeps track of which face to turn on as well as its corresponding color
        // return value of the method is the number of faces to turn on
        virtual int update(int ms, int retIndices[], uint32_t retColors[]) = 0;
        virtual int stop(int retIndices[]) = 0;
        // Set the animation source tag
        void setTag(AnimationTag _tag);
        // sets all of the LEDs that satisfy the face mask (eg: all LEDs on = 0x000FFFFF) to the given color and then stores this information in retIndices and retColors
        int setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]);
        // sets all indices that satisfy the facemask and stores the info in retIndices
        int setIndices(uint32_t faceMask, int retIndices[]);
        void forceFadeOut(int fadeOutTime);

        void updateLEDs(int ms, uint32_t* outDaisyChainColors);
    };

    Animations::AnimationInstance* createAnimationInstance(const Animations::Animation* preset, const DataSet::AnimationBits* bits);
    void destroyAnimationInstance(Animations::AnimationInstance* animationInstance);

}

#pragma pack(pop)
