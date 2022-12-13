#pragma once

namespace DriversNRF::MCUTemperature
{
    void init();

    // Notification management
    typedef void(*TemperatureClientMethod)(void* param, int temperatureTimes100);
    void hook(TemperatureClientMethod method, void* param);
    void unHook(TemperatureClientMethod client);
    void unHookWithParam(void* param);
}
