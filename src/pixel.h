#pragma once

#include <stdint.h>

namespace Pixel
{
    enum RunMode
    {
        RunMode_User = 0,       // Die is in regular mode
        RunMode_Validation,     // Validation mode, blinks ID, etc...
        RunMode_Attract,        // Special logic for displays
        RunMode_Count,
    };

    uint32_t getDeviceID();
    uint32_t getBuildTimestamp();

    RunMode getCurrentRunMode();
    bool setCurrentRunMode(RunMode mode);
}