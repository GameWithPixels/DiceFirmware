#pragma once

#include <stdint.h>

namespace Die
{
    enum TopLevelState
    {
        TopLevel_Unknown = 0,
        TopLevel_SoloPlay,      // Playing animations as a result of landing on faces
        TopLevel_Animator,      // LED Animator
        TopLevel_Testing,       // A number of modules are turned off
    };

    void init();

    void update();

	TopLevelState getCurrentState();
}

