#include "charger_proximity.h"
#include "battery_controller.h"
#include "core/delegate_array.h"

#define MAX_CLIENT_COUNT 2

namespace Modules::ChargerProximity
{
    DelegateArray<ChargerProximityHandler, MAX_CLIENT_COUNT> clients;

    static ChargerProximityState currentProximityState;

    ChargerProximityState computeProximityState(BatteryController::BatteryState state);
	void onBatteryStateChange(void* param, BatteryController::BatteryState newState);

	void init() {
        currentProximityState = computeProximityState(BatteryController::getBatteryState());
        BatteryController::hookBatteryState(onBatteryStateChange, nullptr);
    }

    ChargerProximityState computeProximityState(BatteryController::BatteryState state) {
        ChargerProximityState ret = ChargerProximityState_Off;
        switch (state) {
        case BatteryController::BatteryState_Ok:
        case BatteryController::BatteryState_Low:
		case BatteryController::BatteryState_Error:
            ret = ChargerProximityState_Off;
            break;
		case BatteryController::BatteryState_Charging:
		case BatteryController::BatteryState_Done:
		case BatteryController::BatteryState_BadCharging:
            ret = ChargerProximityState_On;
            break;
        }
        return ret;
    }

	void onBatteryStateChange(void* param, BatteryController::BatteryState newState) {
        auto newProximityState = computeProximityState(newState);
        if (newProximityState != currentProximityState) {
            currentProximityState = newProximityState;
            for (int i = 0; i < clients.Count(); ++i) {
                clients[i].handler(clients[i].token, currentProximityState);
            }
        }
    }

	bool hook(ChargerProximityHandler method, void* param) {
        return clients.Register(param, method);
    }

	void unHook(ChargerProximityHandler method) {
        clients.UnregisterWithHandler(method);
    }

	void unHookWithParam(void* param) {
        clients.UnregisterWithToken(param);
    }
}