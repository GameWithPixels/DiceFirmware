#pragma once

#include "stdint.h"
#include "stddef.h"

namespace Modules::AttractModeController
{
    enum AttractMode : uint8_t
    {
        AttractMode_None = 0,
        AttractMode_Attract
    };

    void init();
}
