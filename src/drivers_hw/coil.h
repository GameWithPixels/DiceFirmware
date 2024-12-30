#pragma once

#include "stdint.h"

namespace DriversHW
{
    namespace Coil
    {
        enum CoilState
        {
            CoilState_Off,
            CoilState_Pinging,
            CoilState_On
        };

        bool init();
        CoilState getCoilState();
        int32_t getVCoilTimes1000();
        int32_t getVCoilMinTimes1000();
        int32_t getVCoilMaxTimes1000();
    }
}

