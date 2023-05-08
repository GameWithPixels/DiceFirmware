#pragma once

namespace DriversNRF::MCUTemperature
{
    void init();

    // Trigger a temperature measurement, use the hook() to get the response
    typedef void(*TemperatureClientMethod)(void* param, int temperatureTimes100);
    bool measure(TemperatureClientMethod callback, void* param);
}
