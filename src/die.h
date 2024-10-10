#pragma once

#include <stdint.h>

namespace Die
{
    void init();
    void update();

    enum UserMode : uint8_t
    {
        UserMode_Unknown = 0,
        UserMode_Default,
        UserMode_RemoteControlled,
    };

    UserMode getCurrentUserMode();
    void beginRemoteControlledMode();
    void exitRemoteControlledMode();
}

