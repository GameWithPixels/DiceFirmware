#pragma once

#include <stdint.h>

namespace Pixel
{
    enum RunMode
    {
        RunMode_Invalid = -1,
        RunMode_User = 0,       // Die is in regular mode
        RunMode_Validation,     // Validation mode, blinks ID, etc...
        RunMode_Attract,        // Special logic for displays
    };

    uint32_t getDeviceID();
    uint32_t getBuildTimestamp();

    RunMode getCurrentMode();
    bool setCurrentMode(RunMode mode);
}