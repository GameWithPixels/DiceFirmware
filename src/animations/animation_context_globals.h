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
        uint16_t normalizedFace;
    };
}

#pragma pack(pop)