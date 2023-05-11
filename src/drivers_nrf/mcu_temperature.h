#pragma once

#include <stdint.h>

namespace DriversNRF::MCUTemperature
{
    void init();
    int32_t measure();
}
