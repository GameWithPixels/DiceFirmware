#pragma once

#include <stdint.h>

namespace Core
{
    struct int3;
}

namespace DriversHW
{
    /// <summary>
    /// The accelerometer I2C devices
    /// </summary>
    namespace AccelChip
    {
        bool init();
        void read(Core::int3* outAccel);

        void enableInterrupt();
        void enableDataInterrupt();
        void disableInterrupt();
        void disableDataInterrupt();
        void clearInterrupt();

        void lowPower();

        // Notification management
        typedef void(*AccelClientMethod)(void* param, const Core::int3& acceleration);
        void hook(AccelClientMethod method, void* param);
        void unHook(AccelClientMethod client);
        void unHookWithParam(void* param);
    }
}

