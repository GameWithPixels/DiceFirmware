#pragma once

#include <stdint.h>
namespace Modules::UserModeController
{
    enum UserMode : uint8_t
    {
        UserMode_Unknown = 0,
        UserMode_Default,
        UserMode_RemoteControlled,
    };

    void init();
    UserMode getCurrentUserMode();
}
