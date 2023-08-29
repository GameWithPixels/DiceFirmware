#pragma once

#include "stdint.h"
#include "profile/profile_buffer.h"
#include "animation_parameters.h"

#pragma pack(push, 1)

namespace Animations
{
    struct AnimationContextGlobals
    {
        uint16_t normalizedFace;
    };
}