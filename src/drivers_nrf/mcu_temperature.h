#pragma once

namespace DriversNRF::MCUTemperature
{
    void init();

    // Trigger a temperature measurement, use the hook() to get the response
    bool measure();

    // Notification management
    typedef void(*TemperatureClientMethod)(void* param, int temperatureTimes100);
    void hook(TemperatureClientMethod method, void* param);
    void unHook(TemperatureClientMethod client);
    void unHookWithParam(void* param);
}
