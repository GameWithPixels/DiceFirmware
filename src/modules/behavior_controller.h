#pragma once

/// <summary>
/// Manages behavior conditions and triggers actions
/// </summary>
namespace Modules::BehaviorController
{
    void init(bool enableAccelerometerRules, bool enableBatteryRules, bool enableConnectionRules);
    
    void onPixelInitialized();
    bool forceCheckBatteryState();

    void DisableAccelerometerRules();
    void EnableAccelerometerRules();

    void DisableBatteryRules();
    void EnableBatteryRules();

    void DisableConnectionRules();
    void EnableConnectionRules();
}
