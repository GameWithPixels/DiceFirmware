#pragma once

/// <summary>
/// </summary>
namespace Modules::LEDErrorIndicator
{
    enum ErrorType
    {
        ErrorType_None = 0,
        ErrorType_LEDs,
        ErrorType_BatterySense,
        ErrorType_BatteryCharge,
        ErrorType_Accelerometer,
        ErrorType_NTC,
    };

    void ShowErrorAndHalt(ErrorType error);
}
