#include "who_are_you.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_nrf/flash.h"
#include "nrf_log.h"
#include "config/settings.h"
#include "data_set/data_set.h"

using namespace Bluetooth;
using namespace Modules;
using namespace Config;
using namespace DriversNRF;

namespace Handlers::WhoAreYou
{
    void whoAreYouHandler(const Message* message) {
        // Central asked for the die state, return it!
        Bluetooth::MessageIAmADie msg;

#if LEGACY_IMADIE_MESSAGE
        msg.ledCount = (uint8_t)SettingsManager::getLayout()->ledCount;
        msg.colorway = SettingsManager::getColorway();
        msg.dataSetHash = DataSet::dataHash();
        msg.pixelId = Pixel::getDeviceID();
        msg.availableFlash = DataSet::availableDataSize();
        msg.buildTimestamp = Pixel::getBuildTimestamp();
        msg.rollState = Accelerometer::currentRollState();
        msg.rollFace = Accelerometer::currentFace();
        msg.batteryLevelPercent = BatteryController::getLevelPercent();
        msg.batteryState = BatteryController::getBatteryState();
        msg.dieType = SettingsManager::getDieType();
#else
        // Die info
        auto settings = SettingsManager::getSettings();
        msg.dieInfo.pixelId = Pixel::getDeviceID();
        msg.dieInfo.chipModel = ChipModel_nRF52810;
        msg.dieInfo.dieType = SettingsManager::getDieType();
        msg.dieInfo.ledCount = (uint8_t)SettingsManager::getLayout()->ledCount;
        msg.dieInfo.colorway = SettingsManager::getColorway();
        msg.dieInfo.runMode = Pixel::getCurrentRunMode();
        memset(msg.customDesignAndColorName.name, 0, sizeof(msg.customDesignAndColorName.name));
        memset(msg.dieName.name, 0, sizeof(msg.dieName.name));
        strncpy(msg.dieName.name, settings->name, sizeof(msg.dieName.name)); // No need to add the null terminator

        // Settings info
        msg.settingsInfo.profileDataHash = DataSet::dataHash();
        msg.settingsInfo.availableFlash = DataSet::availableDataSize();
        msg.settingsInfo.totalUsableFlash = Flash::getUsableBytes();

        // Status info
        msg.statusInfo.batteryLevelPercent = BatteryController::getLevelPercent();
        msg.statusInfo.batteryState = BatteryController::getBatteryState();
        msg.statusInfo.rollState = Accelerometer::currentRollState();
        msg.statusInfo.rollFace = Accelerometer::currentFace();
#endif
        MessageService::SendMessage(&msg);
    }

    void init() {
        // We always send battery events over Bluetooth when connected
        MessageService::RegisterMessageHandler(Message::MessageType_WhoAreYou, whoAreYouHandler);
    }

}
