#pragma once

/// <summary>
/// Manages behavior conditions and triggers actions
/// </summary>
namespace Modules::BehaviorController
{
	void init();
	void onPixelInitialized();
	bool forceCheckBatteryState();

	void DisableAccelerometerRules();
	void EnableAccelerometerRules();
}
