#include "bluetooth_stack.h"
#include "bluetooth_message_service.h"
#include "bluetooth_custom_advertising_data.h"
#include "modules/accelerometer.h"
#include "modules/battery_controller.h"
#include "config/board_config.h"
#include "config/dice_variants.h"
#include "die.h"
#include "ble_advdata.h"
#include "ble_advertising.h"

using namespace Config;
using namespace Modules;

namespace Bluetooth::CustomAdvertisingDataHandler
{
#pragma pack( push, 1)
    // Custom advertising data, so the Pixel app can identify dice before they're even connected
    struct CustomManufacturerData
    {
        uint8_t ledCount;
        uint8_t designAndColor;
        Accelerometer::RollState rollState; // Indicates whether the dice is being shaken, 8 bits
        uint8_t currentFace; // Which face is currently up
        uint8_t batteryLevelAndCharging; // Charge level in percent, MSB is charging
    };
#pragma pack(pop)

    // Global custom manufacturer and service data
    static CustomManufacturerData customManufacturerData;

    void onRollStateChange(void* param, Accelerometer::RollState prevState, int prevFace, Accelerometer::RollState newState, int newFace);
    void onBatteryStateChange(void *param, BatteryController::BatteryState state);
    void onBatteryLevelChange(void *param, uint8_t levelPercent);
    void updateCustomAdvertisingDataState(Accelerometer::RollState newState, int newFace);
    void updateCustomAdvertisingDataBattery(uint8_t batteryValue, uint8_t mask);

    void init() {
        // Set custom advertising data values to 0
        // Actual values will be updated on start()
        memset(&customManufacturerData, 0, sizeof(customManufacturerData));
    }

    bool isChargingOrDone(BatteryController::BatteryState state) {
        return state >= BatteryController::BatteryState_Charging && state <= BatteryController::BatteryState_Done;
    }

    void start() {
        // Initialize the custom advertising data
        customManufacturerData.ledCount = Config::BoardManager::getBoard()->ledCount;
        customManufacturerData.designAndColor = (SettingsManager::getDieType() << 4) | SettingsManager::getColorway();
        customManufacturerData.currentFace = Accelerometer::currentFace();
        customManufacturerData.rollState = Accelerometer::currentRollState();
        customManufacturerData.batteryLevelAndCharging =
            (BatteryController::getLevelPercent() & 0x7F)
            | (isChargingOrDone(BatteryController::getBatteryState()) ? 0x80 : 0);

        Bluetooth::Stack::updateCustomAdvertisingData(
            (uint8_t *)&customManufacturerData, sizeof(customManufacturerData));

        // Register to be notified of accelerometer changes
        Accelerometer::hookRollState(onRollStateChange, nullptr);

        // And battery events too
        BatteryController::hookBatteryState(onBatteryStateChange, nullptr);
        BatteryController::hookLevel(onBatteryLevelChange, nullptr);
    }

    void stop() {
        // Unhook from accelerometer events, we don't need them
        Accelerometer::unHookRollState(onRollStateChange);

        // Unhook battery events too
        BatteryController::unHookBatteryState(onBatteryStateChange);
        BatteryController::unHookLevel(onBatteryLevelChange);
    }

    void onBatteryStateChange(void *param, BatteryController::BatteryState state) {
        updateCustomAdvertisingDataBattery(isChargingOrDone(state) ? 0x80 : 0, 0x7F);
    }

    void onBatteryLevelChange(void *param, uint8_t levelPercent) {
        updateCustomAdvertisingDataBattery(levelPercent & 0x7F, 0x80);
    }

    void onRollStateChange(void* param, Accelerometer::RollState prevState, int prevFace, Accelerometer::RollState newState, int newFace) {
        updateCustomAdvertisingDataState(newState, newFace);
    }

    void updateCustomAdvertisingDataBattery(uint8_t batteryValue, uint8_t mask) {
        customManufacturerData.batteryLevelAndCharging &= mask;
        customManufacturerData.batteryLevelAndCharging |= batteryValue;
        Bluetooth::Stack::updateCustomAdvertisingData((uint8_t*)&customManufacturerData, sizeof(customManufacturerData));
    }

    void updateCustomAdvertisingDataState(Accelerometer::RollState newState, int newFace) {
        // Update manufacturer specific advertising data
        customManufacturerData.currentFace = newFace;
        customManufacturerData.rollState = newState;
        Bluetooth::Stack::updateCustomAdvertisingData((uint8_t*)&customManufacturerData, sizeof(customManufacturerData));
    }
}
