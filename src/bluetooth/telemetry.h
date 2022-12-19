#pragma once

#include "stdint.h"

namespace Bluetooth::Telemetry
{
    void init();
    void start(bool repeat, uint32_t maxRate);
    void stop();
}
