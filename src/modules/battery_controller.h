#pragma once

#include <stdint.h>

/// <summary>
/// Manages a set of running animations, talking to the LED controller
/// to tell it what LEDs must have what intensity at what time.
/// </summary>
namespace Modules::BatteryController
{
	void init();

	enum BatteryState : uint8_t
	{
		BatteryState_Unknown,
		BatteryState_Ok,			// Battery looks fine, nothing is happening
		BatteryState_Low,			// Battery level is low, notify user they should recharge
		BatteryState_Transition,	// Coil voltage is bad, but we don't know yet if that's because we removed the die and
									// the coil cap is still discharging, or if indeed the die is incorrectly positioned
		BatteryState_BadCharging,	// Coil voltage is bad, die is probably positioned incorrectly
									// Note that currently this state is triggered during transition between charging and not charging...
		BatteryState_Error,			// Charge state doesn't make sense (charging but no coil voltage detected for instance)
		BatteryState_Charging,		// Battery is currently recharging
		BatteryState_TrickleCharge, // Battery is almost full
		BatteryState_Done,			// Battery is full and finished charging
		BatteryState_LowTemp,		// Battery is too cold
		BatteryState_HighTemp,		// Battery is too hot
	};

	BatteryState getBatteryState();
	uint8_t getLevelPercent();
	uint16_t getVoltageMilli();
	uint16_t getCoilVoltageMilli();

	void slowMode(bool slow);

	typedef void(*BatteryStateChangeHandler)(void* param, BatteryState newState);
	void hook(BatteryStateChangeHandler method, void* param);
	void unHook(BatteryStateChangeHandler client);
	void unHookWithParam(void* param);

	typedef void(*BatteryLevelChangeHandler)(void* param, uint8_t levelPercent);
	void hookLevel(BatteryLevelChangeHandler method, void* param);
	void unHookLevel(BatteryLevelChangeHandler method);
	void unHookLevelWithParam(void* param);
}
