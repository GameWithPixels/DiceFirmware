#pragma once

#include <stdint.h>

namespace Modules::Temperature
{
    typedef void (*InitCallback)(bool result);
    void init(InitCallback callback);

    int16_t getMCUTemperatureTimes100();
    int16_t getNTCTemperatureTimes100();
    void slowMode(bool slow);

    typedef void(*TemperatureChangeClientMethod)(void* param, int32_t mcuTempTimes100, int32_t ntcTempTimes100);
    bool hookTemperatureChange(TemperatureChangeClientMethod method, void* param);
    void unHookTemperatureChange(TemperatureChangeClientMethod client);
    void unHookTemperatureChangeWithParam(void* param);
}