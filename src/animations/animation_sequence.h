#pragma once

#include "animations/animation.h"
#include "animations/animation_parameters.h"

#pragma pack(push, 1)

namespace Animations
{

    // Defines how the animation layer is blended with existing led colors
    enum AnimationLayerBlendMode : uint8_t
    {
        AnimationLayerBlendMode_Add = 0,
        AnimationLayerBlendMode_Multiply,
        // etc...
    };

    /// <summary>
    /// Stores a list of animations and blends them together
    /// </summary>
    struct AnimationSequence
        : public Animation
    {
        // An animation layer stores a clip and associated data.
        struct AnimationOccurrence
        {
            AnimationPtr animation;
            DScalarPtr startDelay;
            AnimationLayerBlendMode blendMode;
        };

        // An animation is a composition/superposition of layers
        Profile::Array<AnimationOccurrence> layers;
    };

    /// <summary>
    /// instance data for a sequence animation
    /// </summary>
    struct AnimationSequenceInstance
        : public AnimationInstance
    {
        // Setup by the context at creation
        AnimationInstance** layerInstances;

        virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
        virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);

    private:
        const AnimationSequence* getPreset() const {
            return static_cast<const AnimationSequence*>(animationPreset);
        }
    };

}

#pragma pack(pop)
