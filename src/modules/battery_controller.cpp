#include "battery_controller.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_nrf/timers.h"
#include "drivers_hw/battery.h"
#include "drivers_hw/ntc.h"
#include "config/settings.h"
#include "nrf_log.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "die.h"
#include "drivers_nrf/timers.h"
#include "utils/utils.h"
#include "drivers_nrf/a2d.h"
#include "leds.h"
#include "animations\blink.h"

using namespace DriversHW;
using namespace DriversNRF;
using namespace Bluetooth;
using namespace Config;
using namespace Utils;

#define BATTERY_TIMER_MS 1000	// ms
#define BATTERY_TIMER_MS_SLOW 5000	// ms
#define BATTERY_TIMER_MS_QUICK 100 //ms
#define MAX_BATTERY_CLIENTS 3
#define MAX_LEVEL_CLIENTS 2
#define OFF_VCOIL_THRESHOLD 500 //0.5V
#define CHARGE_VCOIL_THRESHOLD 300 //at least 0.3V above VBat
#define VBAT_LOOKUP_SIZE 11
#define BATTERY_ALMOST_EMPTY_PCT 10 // 10%
#define BATTERY_ALMOST_FULL_PCT 95 // 95%
#define TRANSITION_TOO_LONG_DURATION_MS 10000 // 10s
#define TEMPERATURE_TOO_COLD_ENTER 0.0f // degrees C
#define TEMPERATURE_TOO_COLD_LEAVE 5.0f // degrees C
#define TEMPERATURE_TOO_HOT_ENTER 45.0f // degrees C
#define TEMPERATURE_TOO_HOT_LEAVE 40.0f // degrees C
#define LEVEL_SMOOTHING_RATIO 5

namespace Modules::BatteryController
{
    void readBatteryValues();
    void update(void* context);
    void onBatteryEventHandler(void* context, Battery::ChargingEvent evt);
    void onLEDPowerEventHandler(void* context, bool powerOn);
    void updateLevelPercent();
    BatteryState computeNewBatteryState();

    static BatteryState currentBatteryState = BatteryState_Unknown;
    static uint32_t currentBatteryStateStartTime = 0; // Time when we switched to the current battery state

    enum BatteryTemperatureState
    {
        BatteryTemperatureState_Disabled = 0,
        BatteryTemperatureState_Normal,
        BatteryTemperatureState_Low,
        BatteryTemperatureState_Hot,
    };

    static BatteryTemperatureState currentBatteryTempState = BatteryTemperatureState_Normal;

	static DelegateArray<BatteryStateChangeHandler, MAX_BATTERY_CLIENTS> clients;
    static DelegateArray<BatteryLevelChangeHandler, MAX_LEVEL_CLIENTS> levelClients;

    static uint16_t vBatMilli = 0;
    static uint16_t vCoilMilli = 0;
    static uint32_t smoothedLevel = 0;
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
        LEDs::hookPowerState(onLEDPowerEventHandler, nullptr);

        float ntc = NTC::getNTCTemperature();
        if (ntc <= -20.0f || ntc >= 100.0f) {
            currentBatteryTempState = BatteryTemperatureState_Disabled;
            NRF_LOG_INFO("  Battery ntc invalid, temp: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(ntc));
        } else if (ntc < TEMPERATURE_TOO_COLD_ENTER) {
            currentBatteryTempState = BatteryTemperatureState_Low;
            Battery::setDisableChargingOverride(true);
            NRF_LOG_INFO("  Battery too cold! Temp: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(ntc));
        } else if (ntc > TEMPERATURE_TOO_HOT_ENTER) {
            currentBatteryTempState = BatteryTemperatureState_Hot;
            Battery::setDisableChargingOverride(true);
            NRF_LOG_INFO("  Battery too hot! Temp: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(ntc));
        } else {
            currentBatteryTempState = BatteryTemperatureState_Normal;
        }
        // Other values (voltage, vcoil) already displayed by Battery::init()

        // Set initial battery state
        currentBatteryState = computeNewBatteryState();
        currentBatteryStateStartTime = DriversNRF::Timers::millis();
        
		Timers::createTimer(&batteryControllerTimer, APP_TIMER_MODE_SINGLE_SHOT, update);
		Timers::startTimer(batteryControllerTimer, APP_TIMER_TICKS(batteryTimerMs), NULL);

        NRF_LOG_INFO("Battery Controller init");
        NRF_LOG_INFO("  Battery: %d%%", levelPercent);
    }

    void readBatteryValues() {
        vBatMilli = Battery::checkVBat() * 1000;
        vCoilMilli = Battery::checkVCoil() * 1000;
        charging = Battery::checkCharging();
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

    BatteryState computeNewBatteryState() {
        BatteryState ret = BatteryState_Unknown;

        // Figure out a battery charge level
        enum CapacityState
        {
            AlmostEmpty,    // Battery is low
            Average,
            AlmostFull
        };
        CapacityState capacityState = CapacityState::Average;
        if (levelPercent < BATTERY_ALMOST_EMPTY_PCT) {
            capacityState = CapacityState::AlmostEmpty;
        } else if (levelPercent> BATTERY_ALMOST_FULL_PCT) {
            capacityState = CapacityState::AlmostFull;
        }

        enum CoilState
        {
            NotOnCoil,
            OnCoil_Error,
            OnCoil
        };
        CoilState coilState = CoilState::OnCoil_Error;
        if (vCoilMilli < OFF_VCOIL_THRESHOLD) {
            coilState = CoilState::NotOnCoil;
        } else if (vCoilMilli > vBatMilli + CHARGE_VCOIL_THRESHOLD) {
            coilState = CoilState::OnCoil;
        }

        switch (currentBatteryTempState) {
            case BatteryTemperatureState_Normal:
            case BatteryTemperatureState_Disabled:
            default:
            switch (coilState) {
                case CoilState::NotOnCoil:
                default:
                    if (charging) {
                        // Battery is charging but we're not detecting any coil voltage? How is that possible?
                        NRF_LOG_ERROR("Battery Controller: Not on Coil yet still charging?");
                        ret = BatteryState::BatteryState_Error;
                    } else {
                        // Not on charger, not charging, that's perfectly normal, just check the battery level
                        switch (capacityState) {
                            case CapacityState::AlmostEmpty:
                                ret = BatteryState::BatteryState_Low;
                                break;
                            case CapacityState::AlmostFull:
                            case CapacityState::Average:
                            default:
                                ret = BatteryState::BatteryState_Ok;
                                break;
                        }
                    }
                    break;
                case CoilState::OnCoil:
                    if (charging) {
                        switch (capacityState) {
                            case CapacityState::AlmostEmpty:
                            case CapacityState::Average:
                            default:
                                // On charger and charging, good!
                                ret = BatteryState::BatteryState_Charging;
                                break;
                            case CapacityState::AlmostFull:
                                // On charger and almost full, trickle charging
                                ret = BatteryState::BatteryState_TrickleCharge;
                                break;
                        }
                    } else {
                        // On coil but not charging. It's not necessarily an error if charging hasn't started yet or is complete
                        // So check battery level now.
                        switch (capacityState) {
                            case CapacityState::AlmostEmpty:
                                ret = BatteryState::BatteryState_Low;
                                break;
                            case CapacityState::Average:
                            default:
                                ret = BatteryState::BatteryState_Ok;
                                break;
                            case CapacityState::AlmostFull:
                                // On coil, full and not charging? Probably finished charging
                                ret = BatteryState::BatteryState_Done;
                                break;
                        }
                    }
                    break;
                case CoilState::OnCoil_Error:
                    // Incorrectly placed on coil it seems
                    if (currentBatteryState == BatteryState_Transition) {
                        if (Timers::millis() - currentBatteryStateStartTime > TRANSITION_TOO_LONG_DURATION_MS) {
                            // "She's dead Jim!"
                            ret = BatteryState::BatteryState_BadCharging;
                        } else {
                            // It hasn't been long enough to know for sure
                            ret = BatteryState::BatteryState_Transition;
                        }
                    } else if (currentBatteryState == BatteryState::BatteryState_BadCharging) {
                        // We've already determined the die was incorrectly positioned
                        ret = BatteryState::BatteryState_BadCharging;
                    } else {
                        // Coil voltage is bad, but we don't know yet if that's because we removed the die and
                        // the coil cap is still discharging, or if indeed the die is incorrectly positioned
                        ret = BatteryState::BatteryState_Transition;
                    }
                    break;
                }
                break;
            case BatteryTemperatureState_Low:
                ret = BatteryState::BatteryState_LowTemp;
                break;
            case BatteryTemperatureState_Hot:
                ret = BatteryState::BatteryState_HighTemp;
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

        float temperature = NTC::getNTCTemperature();
        //NRF_LOG_INFO("Battery Temperature: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(temperature));
        switch (currentBatteryTempState) {
            case BatteryTemperatureState_Disabled:
                // Don't do anything
                break;
            case BatteryTemperatureState_Normal:
                if (temperature > TEMPERATURE_TOO_HOT_ENTER) {
                    currentBatteryTempState = BatteryTemperatureState_Hot;
                    Battery::setDisableChargingOverride(true);
                    NRF_LOG_INFO("Battery too hot, Preventing Charge");
                } else if (temperature < TEMPERATURE_TOO_COLD_ENTER) {
                    currentBatteryTempState = BatteryTemperatureState_Low;
                    Battery::setDisableChargingOverride(true);
                    NRF_LOG_INFO("Battery too cold, Preventing Charge");
                }
                break;
            case BatteryTemperatureState_Hot:
                if (temperature < TEMPERATURE_TOO_HOT_LEAVE) {
                    currentBatteryTempState = BatteryTemperatureState_Normal;
                    Battery::setDisableChargingOverride(false);
                    NRF_LOG_INFO("Battery cooled down, Allowing Charge");
                }
                break;
            case BatteryTemperatureState_Low:
                if (temperature > TEMPERATURE_TOO_COLD_LEAVE) {
                    currentBatteryTempState = BatteryTemperatureState_Normal;
                    Battery::setDisableChargingOverride(false);
                    NRF_LOG_INFO("Battery warmed up, Allowing Charge");
                }
                break;
        }

        auto newState = computeNewBatteryState();
        if (newState != currentBatteryState) {
            // Update current state time
            currentBatteryStateStartTime = DriversNRF::Timers::millis();
            switch (newState) {
                case BatteryState_Done:
                    NRF_LOG_INFO("Battery finished charging");
                    break;
                case BatteryState_TrickleCharge:
                    NRF_LOG_INFO("Battery is trickle charging");
                    break;
                case BatteryState_Ok:
                    NRF_LOG_INFO("Battery is now Ok");
                    break;
                case BatteryState_Charging:
                    NRF_LOG_INFO("Battery is now Charging");
                    break;
                case BatteryState_Transition:
                    NRF_LOG_INFO("Die is being moved on/off charger");
                    break;
                case BatteryState_BadCharging:
                    NRF_LOG_ERROR("Die is likely poorly positioned on charger");
                    break;
                case BatteryState_Low:
                    NRF_LOG_INFO("Battery is Low");
                    break;
                case BatteryState_Error:
                    NRF_LOG_INFO("Battery is in an error state");
                    break;
                case BatteryState_LowTemp:
                    NRF_LOG_INFO("Battery is too cold");
                    break;
                case BatteryState_HighTemp:
                    NRF_LOG_INFO("Battery is too hot");
                    break;
                default:
                    NRF_LOG_INFO("Battery state is Unknown");
                    break;
            }
            NRF_LOG_INFO("    vBat: %d mV", vBatMilli);
            NRF_LOG_INFO("    vCoil: %d mV", vCoilMilli);
            NRF_LOG_INFO("    charging: %d", charging);
            NRF_LOG_INFO("    level: %d%%", levelPercent);

            currentBatteryState = newState;
            for (int i = 0; i < clients.Count(); ++i) {
    			clients[i].handler(clients[i].token, newState);
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
            batteryTimerMs = BATTERY_TIMER_MS_SLOW;
        } else {
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

            // If it's been too long since we checked, check right away
            uint32_t delay = batteryTimerMs;
            if (DriversNRF::Timers::millis() - currentBatteryStateStartTime > batteryTimerMs) {
                delay = BATTERY_TIMER_MS_QUICK;
            }
            // Restart the timer
		    Timers::startTimer(batteryControllerTimer, APP_TIMER_TICKS(delay), NULL);
        }
    }

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hook(BatteryStateChangeHandler callback, void* parameter) {
		if (!clients.Register(parameter, callback))
		{
			NRF_LOG_ERROR("Too many battery level hooks registered.");
		}
	}

	/// <summary>
	/// </summary>
	void unHook(BatteryStateChangeHandler callback) {
		clients.UnregisterWithHandler(callback);
	}

	/// <summary>
	/// </summary>
	void unHookWithParam(void* param) {
		clients.UnregisterWithToken(param);
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

        // Update the smooth level
        uint32_t newSmoothedLevel = (uint32_t)measuredLevel << 24;
        if (smoothedLevel == 0) {
            smoothedLevel = newSmoothedLevel;
        }
        else {
            // Slowly change the smoothed level
            int diff = (int)newSmoothedLevel - smoothedLevel;
            smoothedLevel += diff / LEVEL_SMOOTHING_RATIO;
        }

        // Round smoothed level into a percentage value
        levelPercent = (smoothedLevel + ((uint32_t)1 << 23)) >> 24;
    }
}
