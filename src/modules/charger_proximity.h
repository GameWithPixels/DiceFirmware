#pragma once

/// <summary>
/// Manages whether die is on charger or not
/// </summary>
namespace Modules::ChargerProximity
{
    enum ChargerProximityState
    {
        ChargerProximityState_On,
        ChargerProximityState_Off
    };

	void init();

	typedef void(*ChargerProximityHandler)(void* param, ChargerProximityState newState);
	bool hook(ChargerProximityHandler method, void* param);
	void unHook(ChargerProximityHandler client);
	void unHookWithParam(void* param);
}
