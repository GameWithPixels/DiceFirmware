#pragma once

#include "stdint.h"
#include "app_util.h"

namespace Bluetooth
{
    namespace CustomAdvertisingDataHandler
    {
        void init();
        void start();
        void stop();
    }
}