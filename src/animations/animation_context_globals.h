#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "animation_parameters.h"

#pragma pack(push, 1)

namespace Animations
{
    // Simple storage for global values useful to the animation system
    // This will be used in the animation context
    struct AnimationContextGlobals
    {
        uint8_t ledCount;                   // Number of LEDs
        uint8_t currentFace;                // Current face index

        uint16_t normalizedCurrentFace;     // Normalized face index
        uint16_t normalizedAnimationTime;   // Normalized time since the start of the animation
    };
}

#pragma pack(pop)
