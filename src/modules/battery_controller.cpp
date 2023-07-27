#include "battery_controller.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_nrf/timers.h"
#include "drivers_hw/battery.h"
#include "drivers_hw/ntc.h"
#include "config/settings.h"
#include "nrf_log.h"
#include "utils/utils.h"
#include "leds.h"
#include "temperature.h"

using namespace DriversHW;
using namespace DriversNRF;
using namespace Bluetooth;
using namespace Config;
using namespace Utils;

#define BATTERY_TIMER_MS 300	// ms
#define BATTERY_TIMER_MS_SLOW 5000	// ms

#define MAX_STATE_CLIENTS 2
#define MAX_BATTERY_CLIENTS 4
#define MAX_LEVEL_CLIENTS 2
#define VCOIL_ON_THRESHOLD 300 //0.3V
#define VCOIL_OFF_THRESHOLD 4000 //4.0V
#define CHARGE_VCOIL_THRESHOLD 300 //at least 0.3V above VBat
#define VBAT_LOOKUP_SIZE 11
#define BATTERY_EMPTY_PCT 1 // 1%
#define BATTERY_LOW_PCT 10 // 10%
#define BATTERY_ALMOST_FULL_PCT 99 // 99%
#define TRANSITION_ON_TOO_LONG_DURATION_MS 1000 // 1s
#define TRANSITION_ON_TOO_LONG_DURATION_SLOW_MS (BATTERY_TIMER_MS_SLOW + 100) // 5s
#define TRANSITION_OFF_TOO_LONG_DURATION_MS 5000 // 5s
#define TRANSITION_OFF_TOO_LONG_DURATION_SLOW_MS (BATTERY_TIMER_MS_SLOW + 100) // 5s
#define TEMPERATURE_TOO_COLD 0   // degrees C
#define TEMPERATURE_TOO_HOT 4500 // degrees C
#define TEMPERATURE_COOLDOWN_ENTER 4100 // degrees C
#define TEMPERATURE_COOLDOWN_LEAVE 3900 // degrees C
#define LEVEL_SMOOTHING_RATIO 5
#define MAX_V_MILLIS 10000
#define MAX_CHARGE_START_TIME 3000 //ms
#define MAX_CHARGE_START_TIME_SLOW (BATTERY_TIMER_MS_SLOW + 100) //ms
#define VBAT_MEASUREMENT_THRESHOLD 50 //mV

namespace Modules::BatteryController
{
    void readBatteryValues();
    void update(void* context);
    void onBatteryEventHandler(void* context, Battery::ChargingEvent evt);
    void onLEDPowerEventHandler(void* context, bool powerOn);
    void updateLevelPercent();
    State computeNewState();
    BatteryState computeNewBatteryState();

    void onEnableChargingHandler(const Message *msg);
    void onDisableChargingHandler(const Message *msg);

    enum UpdateRate
    {
        UpdateRate_Normal,
        UpdateRate_Slow
    };

    enum CapacityState
    {
        CapacityState_Empty,          // About to turn off
        CapacityState_Low,            // Battery is low
        CapacityState_Average,
        CapacityState_Full
    };

    enum CoilState
    {
        CoilState_Low,
        CoilState_Medium,
        CoilState_High
    };

    static UpdateRate currentUpdateRate = UpdateRate_Normal;
    static State currentState = State_Unknown;
    static uint32_t currentStateStartTime = 0; // Time when we switched to the current battery state

    enum BatteryTemperatureState
    {
        BatteryTemperatureState_Disabled = 0,
        BatteryTemperatureState_Normal,
        BatteryTemperatureState_Cooldown,
        BatteryTemperatureState_Low,
        BatteryTemperatureState_Hot,
    };

    static BatteryTemperatureState currentBatteryTempState = BatteryTemperatureState_Normal;

	static DelegateArray<BatteryControllerStateChangeHandler, MAX_STATE_CLIENTS> clients;
    static DelegateArray<BatteryStateChangeHandler, MAX_BATTERY_CLIENTS> batteryClients;
    static DelegateArray<BatteryLevelChangeHandler, MAX_LEVEL_CLIENTS> levelClients;

    static BatteryState currentBatteryState = BatteryState_Ok;
    static uint16_t vBatMilli = 0;
    static uint16_t vCoilMilli = 0;
    // static uint32_t smoothedLevel = 0;
    static uint8_t levelPercent = 0;
    static bool charging = false;
    static uint16_t batteryTimerMs = BATTERY_TIMER_MS;

    APP_TIMER_DEF(batteryControllerTimer);

    struct VoltageAndLevels
    {
        uint16_t voltageMilli;
        uint8_t levelPercent[2]; // index 0 is when discharging, index 1 is when charging
    };

    // This lookup table defines our voltage to capacity curves, both when charging (values are higher)
    // and discharging (values are lower).
    static const VoltageAndLevels lookup[VBAT_LOOKUP_SIZE] =
    {
        {4100, {100, 100}},
        {4000, {100,  97}},
        {3900, { 93,  88}},
        {3800, { 80,  70}},
        {3700, { 60,  48}},
        {3600, { 33,  14}},
        {3500, { 16,   6}},
        {3400, {  9,   3}},
        {3300, {  5,   2}},
        {3200, {  3,   1}},
        {3000, {  0,   0}},
    };

    void init() {
        readBatteryValues();
        updateLevelPercent();

        // Register for battery events
        Battery::hook(onBatteryEventHandler, nullptr);

        // Register for led events
        //LEDs::hookPowerState(onLEDPowerEventHandler, nullptr);

        int ntcTimes100 = Temperature::getNTCTemperatureTimes100();
        if (ntcTimes100 <= -2000 || ntcTimes100 >= 10000) {
            currentBatteryTempState = BatteryTemperatureState_Disabled;
            NRF_LOG_WARNING("  Battery ntc invalid, temp: %d.%02d", ntcTimes100 / 100, ntcTimes100 % 100);
        } else if (ntcTimes100 < TEMPERATURE_TOO_COLD) {
            currentBatteryTempState = BatteryTemperatureState_Low;
            Battery::setDisableChargingOverride(true);
            NRF_LOG_INFO("  Battery too cold! Temp: %d.%02d", ntcTimes100 / 100, ntcTimes100 % 100);
        } else if (ntcTimes100 > TEMPERATURE_TOO_HOT) {
            currentBatteryTempState = BatteryTemperatureState_Hot;
            Battery::setDisableChargingOverride(true);
            NRF_LOG_INFO("  Battery too hot! Temp: %d.%02d", ntcTimes100 / 100, ntcTimes100 % 100);
        } else {
            currentBatteryTempState = BatteryTemperatureState_Normal;
        }
        // Other values (voltage, vcoil) already displayed by Battery::init()

        currentUpdateRate = UpdateRate_Normal;

        // Set initial battery state
        currentState = computeNewState();
        currentStateStartTime = DriversNRF::Timers::millis();
        currentBatteryState = computeNewBatteryState();
        
		Timers::createTimer(&batteryControllerTimer, APP_TIMER_MODE_SINGLE_SHOT, update);
		Timers::startTimer(batteryControllerTimer, APP_TIMER_TICKS(batteryTimerMs), NULL);

        MessageService::RegisterMessageHandler(Message::MessageType_EnableCharging, onEnableChargingHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_DisableCharging, onDisableChargingHandler);

        NRF_LOG_INFO("Battery Controller init: %d%%", levelPercent);
    }

    void readBatteryValues() {
        // VBat should only be updated if the difference between the stored and measured value is enough
        auto newVBatMilli = clamp<int32_t>(Battery::checkVBatTimes1000(), 0, MAX_V_MILLIS);
        if ((newVBatMilli < vBatMilli - VBAT_MEASUREMENT_THRESHOLD) || (newVBatMilli > vBatMilli + VBAT_MEASUREMENT_THRESHOLD)) {
            vBatMilli = newVBatMilli;
        }
        vCoilMilli = clamp<int32_t>(Battery::checkVCoilTimes1000(), 0, MAX_V_MILLIS);
        charging = clamp<int32_t>(Battery::checkCharging(), 0, MAX_V_MILLIS);
    }

	State getState() {
        return currentState;
    }

    BatteryState getBatteryState(){
        return currentBatteryState;
    }

    uint8_t getLevelPercent() {
        return levelPercent;
    }

    uint16_t getVoltageMilli() {
        return vBatMilli;
    }

    uint16_t getCoilVoltageMilli() {
        return vCoilMilli;
    }

    State computeNewState() {
        State ret = currentState;

        // Figure out a battery charge level
        CapacityState capacityState = CapacityState::CapacityState_Average;
        if (levelPercent < BATTERY_EMPTY_PCT) {
            capacityState = CapacityState::CapacityState_Empty;
        } else if (levelPercent < BATTERY_LOW_PCT) {
            capacityState = CapacityState::CapacityState_Low;
        } else if (levelPercent > BATTERY_ALMOST_FULL_PCT) {
            capacityState = CapacityState::CapacityState_Full;
        }

        CoilState coilState = CoilState::CoilState_Medium;
        if (Battery::getDisableChargingOverride()) {
            // Weird behavior from the circuit makes coil voltage ~2.0V even when off the coil
            // So we can't really know that it's partially off
            if (vCoilMilli < VCOIL_OFF_THRESHOLD) {
                coilState = CoilState::CoilState_Low;
            } else {
                coilState = CoilState::CoilState_High;
            }
        } else {
            if (vCoilMilli < VCOIL_ON_THRESHOLD) {
                coilState = CoilState::CoilState_Low;
            } else if (vCoilMilli > VCOIL_OFF_THRESHOLD) {
                coilState = CoilState::CoilState_High;
            }
        }

        switch (currentBatteryTempState) {
            case BatteryTemperatureState_Normal:
            case BatteryTemperatureState_Cooldown:
            case BatteryTemperatureState_Disabled:
            default:
            switch (coilState) {
                case CoilState::CoilState_Low:
                default:
                    if (charging) {
                        // Battery is charging but we're not detecting any coil voltage? How is that possible?
                        NRF_LOG_ERROR("Battery Controller: Not on Coil yet still charging?");
                        ret = State::State_Error;
                    } else {
                        // Not on charger, not charging, that's perfectly normal, just check the battery level
                        switch (capacityState) {
                            case CapacityState::CapacityState_Empty:
                                ret = State::State_Empty;
                                break;
                            case CapacityState::CapacityState_Low:
                                ret = State::State_Low;
                                break;
                            case CapacityState::CapacityState_Full:
                            case CapacityState::CapacityState_Average:
                            default:
                                ret = State::State_Ok;
                                break;
                        }
                    }
                    break;
                case CoilState::CoilState_High:
                    if (charging) {
                        // On charger and charging, good!
                        switch (capacityState) {
                            case CapacityState::CapacityState_Empty:
                            case CapacityState::CapacityState_Low:
                                ret = State::State_ChargingLow;
                                break;
                            case CapacityState::CapacityState_Full:
                                ret = State::State_Trickle;
                                break;
                            case CapacityState::CapacityState_Average:
                            default:
                                ret = State::State_Charging;
                                break;
                        }
                    } else {
                        // On coil but not charging. It's not necessarily an error if:
                        // - charging hasn't started yet
                        // - charging is complete
                        // - battery is in cooldown mode
                        // So check battery level now.
                        switch (capacityState) {
                            case CapacityState::CapacityState_Empty:
                            case CapacityState::CapacityState_Low:
                            case CapacityState::CapacityState_Average:
                            default:
                                // If we remain in this state too long, it's an error
                                {
                                    // Check temperature state
                                    if (currentBatteryTempState == BatteryTemperatureState::BatteryTemperatureState_Cooldown) {
                                        ret = State::State_Cooldown;
                                    } else {
                                        uint32_t maxTransitionTime = MAX_CHARGE_START_TIME;
                                        if (currentUpdateRate == UpdateRate_Slow) {
                                            maxTransitionTime = MAX_CHARGE_START_TIME_SLOW;
                                        }
                                        if (Timers::millis() - currentStateStartTime > maxTransitionTime) {
                                            // We should have started charging!!!
                                            ret = State::State_Error;
                                        } else {
                                            if (capacityState == CapacityState::CapacityState_Empty) {
                                                ret = State::State_Empty;
                                            } else if (capacityState == CapacityState::CapacityState_Low) {
                                                ret = State::State_Low;
                                            } else {
                                                ret = State::State_Ok;
                                            }
                                        }
                                    }
                                }
                                break;
                            case CapacityState::CapacityState_Full:
                                // On coil, full and not charging? Probably finished charging
                                ret = State::State_Done;
                                break;
                        }
                    }
                    break;
                case CoilState::CoilState_Medium:
                    // Partially on coil
                    switch (currentState) {
                        case State_TransitionOn:
                            {
                                uint32_t maxTransitionTime = TRANSITION_ON_TOO_LONG_DURATION_MS;
                                if (currentUpdateRate == UpdateRate_Slow) {
                                    maxTransitionTime = TRANSITION_ON_TOO_LONG_DURATION_SLOW_MS;
                                }
                                if (Timers::millis() - currentStateStartTime > maxTransitionTime) {
                                    // "She's dead Jim!"
                                    ret = State::State_BadCharging;
                                } // else it hasn't been long enough to know for sure
                            }
                            break;
                        case State_TransitionOff:
                            {
                                uint32_t maxTransitionTime = TRANSITION_OFF_TOO_LONG_DURATION_MS;
                                if (currentUpdateRate == UpdateRate_Slow) {
                                    maxTransitionTime = TRANSITION_OFF_TOO_LONG_DURATION_SLOW_MS;
                                }
                                if (Timers::millis() - currentStateStartTime > maxTransitionTime) {
                                    // "She's dead Jim!"
                                    ret = State::State_BadCharging;
                                } // else it hasn't been long enough to know for sure
                            }
                            break;
		                case State::State_Ok:
		                case State::State_Low:
		                case State::State_Empty:
                            // Not fully on coil yet
                            ret = State_TransitionOn;
                            break;
		                case State::State_ChargingLow:
		                case State::State_Charging:
		                case State::State_Trickle:
		                case State::State_Cooldown:
		                case State::State_Done:
                            // On coil but coming off
                            ret = State_TransitionOff;
                            break;
                        case State::State_BadCharging:
		                case State::State_Unknown:
		                case State::State_Error:
		                case State::State_LowTemp:
		                case State::State_HighTemp:
                            // Already in error state, stay there!
                            break;
                    }
                    break;
                }
                break;
            case BatteryTemperatureState_Low:
                ret = State::State_LowTemp;
                break;
            case BatteryTemperatureState_Hot:
                ret = State::State_HighTemp;
                break;
        }

        return ret;
    }

    BatteryState computeNewBatteryState() {
        BatteryState ret = BatteryState_Ok;
        switch (currentState) {
            case State_Unknown:
            case State_Ok:
                ret = BatteryState_Ok;
                break;
            case State_Empty:
            case State_Low:
                ret = BatteryState_Low;
                break;
            case State_TransitionOn:
                ret = BatteryState_Charging;
                break;
            case State_TransitionOff:
                ret = BatteryState_Ok;
                break;
            case State_BadCharging:
                ret = BatteryState_BadCharging;
                break;
            case State_Error:
            case State_LowTemp:
            case State_HighTemp:
                ret = BatteryState_Error;
                break;
            case State_ChargingLow:
            case State_Charging:
            case State_Cooldown:
                ret = BatteryState_Charging;
                break;
            case State_Trickle:
            case State_Done:
                ret = BatteryState_Done;
                break;
        }
        return ret;
    }

    void update(void* context) {
        // // DEBUG
        // Battery::printA2DReadings();
        // // DEBUG

        // Measure new values
        readBatteryValues();
        uint8_t prevLevel = levelPercent;
        updateLevelPercent();

        int temperatureTimes100 = Temperature::getNTCTemperatureTimes100();
        switch (currentBatteryTempState) {
            case BatteryTemperatureState_Disabled:
                // Don't do anything
                break;
            case BatteryTemperatureState_Normal:
                if (temperatureTimes100 > TEMPERATURE_TOO_HOT) {
                    currentBatteryTempState = BatteryTemperatureState_Hot;
                    Battery::setDisableChargingOverride(true);
                    NRF_LOG_INFO("Battery too hot, Preventing Charge");
                } else if (temperatureTimes100 > TEMPERATURE_COOLDOWN_ENTER) {
                    currentBatteryTempState = BatteryTemperatureState_Cooldown;
                    Battery::setDisableChargingOverride(true);
                    NRF_LOG_INFO("Battery hot, Preventing Charge to cooldown");
                } else if (temperatureTimes100 < TEMPERATURE_TOO_COLD) {
                    currentBatteryTempState = BatteryTemperatureState_Low;
                    Battery::setDisableChargingOverride(true);
                    NRF_LOG_INFO("Battery too cold, Preventing Charge");
                }
                // Else stay in normal mode
                break;
            case BatteryTemperatureState_Cooldown:
                if (temperatureTimes100 > TEMPERATURE_TOO_HOT) {
                    currentBatteryTempState = BatteryTemperatureState_Hot;
                    NRF_LOG_INFO("Battery still getting too hot");
                } else if (temperatureTimes100 < TEMPERATURE_COOLDOWN_LEAVE) {
                    currentBatteryTempState = BatteryTemperatureState_Normal;
                    Battery::setDisableChargingOverride(false);
                    NRF_LOG_INFO("Battery cooled down, Allowing Charge");
                }
                // Else stay in normal mode
                break;
            case BatteryTemperatureState_Hot:
                if (temperatureTimes100 < TEMPERATURE_COOLDOWN_LEAVE) {
                    currentBatteryTempState = BatteryTemperatureState_Normal;
                    Battery::setDisableChargingOverride(false);
                    NRF_LOG_INFO("Battery cooled down, Allowing Charge");
                }
                break;
            case BatteryTemperatureState_Low:
                if (temperatureTimes100 > TEMPERATURE_TOO_COLD) {
                    currentBatteryTempState = BatteryTemperatureState_Normal;
                    Battery::setDisableChargingOverride(false);
                    NRF_LOG_INFO("Battery warmed up, Allowing Charge");
                }
                break;
        }

        auto newState = computeNewState();
        if (newState != currentState) {
            // Update current state time
            currentStateStartTime = DriversNRF::Timers::millis();
            switch (newState) {
                case State_Done:
                    NRF_LOG_INFO("Battery finished charging");
                    break;
                case State_Ok:
                    NRF_LOG_INFO("Battery is now Ok");
                    break;
                case State_ChargingLow:
                    NRF_LOG_INFO("Battery is now Charging, but still low");
                    break;
                case State_Charging:
                    NRF_LOG_INFO("Battery is now Charging");
                    break;
                case State_Cooldown:
                    NRF_LOG_INFO("Battery is now cooling");
                    break;
                case State_Trickle:
                    NRF_LOG_INFO("Battery is now Trickle Charging");
                    break;
                case State_TransitionOn:
                    NRF_LOG_INFO("Die is being moved on charger");
                    break;
                case State_TransitionOff:
                    NRF_LOG_INFO("Die is being moved off charger");
                    break;
                case State_BadCharging:
                    NRF_LOG_ERROR("Die is likely poorly positioned on charger");
                    break;
                case State_Empty:
                    NRF_LOG_INFO("Battery is Empty");
                    break;
                case State_Low:
                    NRF_LOG_INFO("Battery is Low");
                    break;
                case State_Error:
                    NRF_LOG_INFO("Battery is in an error state");
                    break;
                case State_LowTemp:
                    NRF_LOG_INFO("Battery is too cold");
                    break;
                case State_HighTemp:
                    NRF_LOG_INFO("Battery is too hot");
                    break;
                default:
                    NRF_LOG_INFO("Battery state is Unknown");
                    break;
            }
            NRF_LOG_DEBUG("    vBat: %d mV", vBatMilli);
            NRF_LOG_DEBUG("    vCoil: %d mV", vCoilMilli);
            NRF_LOG_DEBUG("    charging: %d", charging);
            NRF_LOG_DEBUG("    level: %d%%", levelPercent);

            currentState = newState;
            for (int i = 0; i < clients.Count(); ++i) {
    			clients[i].handler(clients[i].token, newState);
            }
        }

        auto newBatteryState = computeNewBatteryState();
        if (newBatteryState != currentBatteryState) {
            currentBatteryState = newBatteryState;
            for (int i = 0; i < batteryClients.Count(); ++i) {
    			batteryClients[i].handler(batteryClients[i].token, newBatteryState);
            }
        }

        if (prevLevel != levelPercent) {
            for (int i = 0; i < levelClients.Count(); ++i) {
                levelClients[i].handler(levelClients[i].token, levelPercent);
            }
        }

        Timers::startTimer(batteryControllerTimer, APP_TIMER_TICKS(batteryTimerMs), NULL);
    }

    void slowMode(bool slow) {
        if (slow) {
            currentUpdateRate = UpdateRate_Slow;
            batteryTimerMs = BATTERY_TIMER_MS_SLOW;
        } else {
            currentUpdateRate = UpdateRate_Normal;
            batteryTimerMs = BATTERY_TIMER_MS;
        }
        // The new timer duration will kick in on the next reset of the battery timer.
    }

    void onBatteryEventHandler(void* context, Battery::ChargingEvent evt) {
        update(nullptr);
    }

    void onLEDPowerEventHandler(void* context, bool powerOn) {
        if (powerOn) {
            // Stop reading battery voltage as it may significantly drop when LEDs are turned on
            Timers::stopTimer(batteryControllerTimer);
        } else {
            Timers::stopTimer(batteryControllerTimer);

            // Restart the timer
		    Timers::startTimer(batteryControllerTimer, APP_TIMER_TICKS(batteryTimerMs), NULL);
        }
    }

    void onEnableChargingHandler(const Message *msg) {
        Battery::setDisableChargingOverride(false);
    }

    void onDisableChargingHandler(const Message *msg) {
        Battery::setDisableChargingOverride(true);
    }

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hookControllerState(BatteryControllerStateChangeHandler callback, void* parameter) {
		if (!clients.Register(parameter, callback))
		{
			NRF_LOG_ERROR("Too many battery level hooks registered.");
		}
	}

	/// <summary>
	/// </summary>
	void unHookControllerState(BatteryControllerStateChangeHandler callback) {
		clients.UnregisterWithHandler(callback);
	}

	/// <summary>
	/// </summary>
	void unHookControllerStateWithParam(void* param) {
		clients.UnregisterWithToken(param);
	}

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hookBatteryState(BatteryStateChangeHandler callback, void* parameter) {
		if (!batteryClients.Register(parameter, callback))
		{
			NRF_LOG_ERROR("Too many battery level hooks registered.");
		}
	}

	/// <summary>
	/// </summary>
	void unHookBatteryState(BatteryStateChangeHandler callback) {
		batteryClients.UnregisterWithHandler(callback);
	}

	/// <summary>
	/// </summary>
	void unHookBatteryStateWithParam(void* param) {
		batteryClients.UnregisterWithToken(param);
	}

	/// <summary>
	/// </summary>
	void hookLevel(BatteryLevelChangeHandler callback, void* parameter) {
		if (!levelClients.Register(parameter, callback)) {
			NRF_LOG_ERROR("Too many battery state hooks registered.");
		}
	}

	/// <summary>
	/// </summary>
	void unHookLevel(BatteryLevelChangeHandler callback) {
		levelClients.UnregisterWithHandler(callback);
	}

	/// <summary>
	/// </summary>
	void unHookLevelWithParam(void* param) {
		levelClients.UnregisterWithToken(param);
	}

    void updateLevelPercent() {
        int chargingOffset = charging ? 1 : 0;

        // Find the first voltage that is greater than the measured voltage
        // Because voltages are sorted, we know that we can then linearly interpolate the charge level
        // using the previous and next entries in the lookup table.
        int nextIndex = 0;
        while (nextIndex < VBAT_LOOKUP_SIZE && lookup[nextIndex].voltageMilli >= vBatMilli) {
            nextIndex++;
        }

        uint8_t measuredLevel = 0;
        if (nextIndex == 0) {
            measuredLevel = 100;
        }
        else if (nextIndex == VBAT_LOOKUP_SIZE) {
            measuredLevel = 0;
        }
        else {
            // Grab the prev and next keyframes
            auto next = lookup[nextIndex];
            auto prev = lookup[nextIndex - 1];

            // Compute the interpolation parameter
            int percentMilli = ((int)prev.voltageMilli - (int)vBatMilli) * 1000 / ((int)prev.voltageMilli - (int)next.voltageMilli);
            measuredLevel = ((int)prev.levelPercent[chargingOffset] * (1000 - percentMilli) + (int)next.levelPercent[chargingOffset] * percentMilli) / 1000;
        }
        
        // // Update the smooth level
        // uint32_t newSmoothedLevel = (uint32_t)measuredLevel << 24;
        // if (smoothedLevel == 0) {
        //     smoothedLevel = newSmoothedLevel;
        // }
        // else {
        //     // Slowly change the smoothed level
        //     int diff = (int)newSmoothedLevel - smoothedLevel;
        //     smoothedLevel += diff / LEVEL_SMOOTHING_RATIO;
        // }

        // // Round smoothed level into a percentage value
        // levelPercent = (smoothedLevel + ((uint32_t)1 << 23)) >> 24;
        levelPercent = measuredLevel;
    }
}
