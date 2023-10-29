#pragma once

#include <stdint.h>

/// <summary>
/// Manages a set of running animations, talking to the LED controller
/// to tell it what LEDs must have what intensity at what time.
/// </summary>
namespace Modules::BatteryController
{
	void init();

	enum State : uint8_t
	{
		State_Unknown,
		State_Ok,			// Battery looks fine, nothing is happening
		State_Empty,		// Battery voltage is so low the die might turn off at any time
		State_Low,			// Battery level is low, notify user they should recharge
		State_TransitionOn,	// Coil voltage is bad, but we don't know yet if that's because we just put the die
							// on the coil, or if indeed the die is incorrectly positioned
		State_TransitionOff,	// Coil voltage is bad, but we don't know yet if that's because we removed the die and
							// the coil cap is still discharging, or if indeed the die is incorrectly positioned
		State_BadCharging,	// Coil voltage is bad, die is probably positioned incorrectly
							// Note that currently this state is triggered during transition between charging and not charging...
		State_Error,		// Charge state doesn't make sense (charging but no coil voltage detected for instance)
		State_ChargingLow,	// Battery is currently recharging, but still really low
		State_Charging,		// Battery is currently recharging
		State_Cooldown,		// Battery is cooling down
		State_Trickle,		// Battery is currently recharging, but it's pretty much full!
		State_Done,			// Battery is full and finished charging
		State_LowTemp,		// Battery is too cold
		State_HighTemp,		// Battery is too hot
	};

	State getState();

	enum BatteryState : uint8_t
	{
        BatteryState_Ok,
        BatteryState_Low,
		BatteryState_Charging,
		BatteryState_Done,
		BatteryState_BadCharging,
		BatteryState_Error,
	};

	BatteryState getBatteryState();
	uint8_t getLevelPercent();
	uint16_t getVoltageMilli();
	uint16_t getCoilVoltageMilli();

    enum UpdateRate
    {
        UpdateRate_Normal,
        UpdateRate_Slow,
        UpdateRate_Fast
    };

	void setUpdateRate(UpdateRate rate);

	typedef void(*BatteryControllerStateChangeHandler)(void* param, State newState);
	void hookControllerState(BatteryControllerStateChangeHandler method, void* param);
	void unHookControllerState(BatteryControllerStateChangeHandler client);
	void unHookControllerStateWithParam(void* param);

	typedef void(*BatteryStateChangeHandler)(void* param, BatteryState newState);
	void hookBatteryState(BatteryStateChangeHandler method, void* param);
	void unHookBatteryState(BatteryStateChangeHandler client);
	void unHookBatteryStateWithParam(void* param);

	typedef void(*BatteryLevelChangeHandler)(void* param, uint8_t levelPercent);
	void hookLevel(BatteryLevelChangeHandler method, void* param);
	void unHookLevel(BatteryLevelChangeHandler method);
	void unHookLevelWithParam(void* param);
}
