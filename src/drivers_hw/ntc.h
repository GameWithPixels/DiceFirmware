#pragma once

#include <stdint.h>

namespace DriversHW
{
    namespace NTC
    {
        void init();

        typedef void(*TemperatureClientMethod)(int32_t temperatureTimes100);
        bool measure(TemperatureClientMethod callback);
    }
}

