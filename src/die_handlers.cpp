#include "die.h"
#include "die_private.h"
#include "pixel.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/log.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "config/settings.h"
#include "config/board_config.h"
#include "config/value_store.h"
#include "modules/charger_proximity.h"
#include "modules/anim_controller.h"
#include "modules/behavior_controller.h"
#include "modules/temperature.h"
#include "modules/validation_manager.h"
#include "notifications/battery.h"
#include "notifications/roll.h"
#include "notifications/rssi.h"
#include "data_set/data_set.h"

#define CHARGER_STATE_CHANGE_FADE_OUT_MS 250

using namespace Modules;
using namespace Bluetooth;
using namespace Config;
using namespace DriversNRF;

namespace Die
{
    void whoAreYouHandler(const Message* message) {
        // Central asked for the die state, return it!
        Bluetooth::MessageIAmADie msg;

#if LEGACY_IMADIE_MESSAGE
        msg.ledCount = (uint8_t)BoardManager::getBoard()->ledCount;
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
        msg.dieInfo.ledCount = (uint8_t)BoardManager::getBoard()->ledCount;
        msg.dieInfo.colorway = SettingsManager::getColorway();
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

    void onPowerEvent(PowerManager::PowerManagerEvent event) {
        switch (event) {
            case PowerManager::PowerManagerEvent_PrepareSysOff:
                //NRF_LOG_INFO("Going to system off mode");
                Accelerometer::stop();
                Accelerometer::lowPower();
                AnimController::stop();
                break;
            case PowerManager::PowerManagerEvent_PrepareWakeUp:
                //NRF_LOG_INFO("Going to low power mode");
                Accelerometer::stop();
                Accelerometer::lowPower();
                AnimController::stop();
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Slow);
                Temperature::slowMode(true);
                break;
            case PowerManager::PowerManagerEvent_PrepareSleep:
                //NRF_LOG_INFO("Going to Sleep");
                Accelerometer::stop();
                //AnimController::stop();
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Slow);
                Temperature::slowMode(true);
                Stack::stopAdvertising();

                if (ValidationManager::inValidation()) {
                    // In validation mode we just go to system off mode and rely
                    // on the magnet to power cycle the chip
                    PowerManager::goToSystemOff();
                } else {
                    // Set interrupt pin to wake up power manager
                    Accelerometer::enableInterrupt([](void* param) {
                        Scheduler::push(nullptr, 0, [](void* ignoreData, uint16_t ignoreSize) {
                            // Wake up
                            PowerManager::wakeFromSleep();
                        });
                    }, nullptr);
                }
                break;
            case PowerManager::PowerManagerEvent_WakingUpFromSleep:
                //NRF_LOG_INFO("Resuming from Sleep");
                Accelerometer::wakeUp();
                //AnimController::start();
                BatteryController::setUpdateRate(BatteryController::UpdateRate_Normal);
                Temperature::slowMode(false);
                Stack::startAdvertising();
                break;
            default:
                break;
        }

    }

    void onConnectionEvent(void* token, bool connected) {
        if (!connected) {
            // Return to solo play
            enterStandardState();
            PowerManager::resume();
        } else {
            PowerManager::pause();
        }

        Notifications::Rssi::notifyConnectionEvent(connected);
    }

    void onChargerStateChange(void* param, ChargerProximity::ChargerProximityState newState) {
        switch (newState) {
            case ChargerProximity::ChargerProximityState_Off:
                // Re-enable accelerometer animations
                BehaviorController::EnableAccelerometerRules();
                break;
            case ChargerProximity::ChargerProximityState_On:
                // Disable accelerometer-based animations
                BehaviorController::DisableAccelerometerRules();

                // Kill any currently executing accelerometer-triggered animation
                AnimController::fadeOutAnimsWithTag(
                    Animations::AnimationTag_Accelerometer,
                    CHARGER_STATE_CHANGE_FADE_OUT_MS);
                break;
        }
    }    

    void playLEDAnimHandler(const Message* msg) {
        auto playAnimMessage = (const MessagePlayAnim*)msg;
        NRF_LOG_DEBUG("Playing animation %d", playAnimMessage->animation);
		auto animationPreset = DataSet::getAnimation((int)playAnimMessage->animation);
        AnimController::play(
            animationPreset,
            DataSet::getAnimationBits(),
            playAnimMessage->remapFace,
            playAnimMessage->loop,
            Animations::AnimationTag_BluetoothMessage);
    }

    void stopLEDAnimHandler(const Message* msg) {
        auto stopAnimMessage = (const MessageStopAnim*)msg;
        NRF_LOG_DEBUG("Stopping animation %d", stopAnimMessage->animation);
		// Find the preset for this animation Index
		auto animationPreset = DataSet::getAnimation((int)stopAnimMessage->animation);
        AnimController::stop(animationPreset, stopAnimMessage->remapFace);
    }

    void stopAllLEDAnimsHandler(const Message* msg) {
        NRF_LOG_DEBUG("Stopping all animations");
        AnimController::stopAll();
    }

    void setTopLevelStateHandler(const Message *msg) {
        auto setTopLevelStateMessage = (const MessageSetTopLevelState *)msg;
        switch (setTopLevelStateMessage->state) {
            case TopLevel_SoloPlay:
                enterStandardState();
                break;
            case TopLevel_Animator:
                enterLEDAnimState();
                break;
            case TopLevel_Testing:
                enterTestingState();
                break;
            default:
                break;
        }
    }

    void powerOperationHandler(const Message* msg) {
        auto powerMsg = (const MessagePowerOperation *)msg;
        switch (powerMsg->operation)
        {
            case PowerMode_TurnOff:
                PowerManager::goToSystemOff();
                break;
            case PowerMode_Reset:
                PowerManager::reset();
                break;
            case PowerOperation_Sleep:
                Stack::sleepOnDisconnect();
                Stack::disconnect();
                break;
        }
    }

    void clearSettingsHandler(const Message* msg) {
        SettingsManager::programDefaults([](bool ignore) {
            MessageService::SendMessage(Bluetooth::Message::MessageType_ClearSettingsAck);
        });
    }

    void storeValueHandler(const Message *msg) {
        auto powerMsg = (const MessageStoreValue *)msg;

        MessageStoreValueAck ack;
        ack.result = StoreValueResult_UnknownError;
        ack.index = -1;

        if (powerMsg->value) {
            const auto result = ValueStore::writeValue(powerMsg->value);
            if (result >= 0) {
                ack.index = result;
                ack.result = StoreValueResult_Success;
            } else {
                switch (result) {
                case ValueStore::WriteValueError_StoreFull:
                    ack.result = StoreValueResult_StoreFull;
                    break;
                case ValueStore::WriteValueError_NotPermited:
                    ack.result = StoreValueResult_NotPermitted;
                    break;
                }
            }
        } else {
            ack.result = StoreValueResult_InvalidRange;
        }

        // Send ACK
        MessageService::SendMessage(&ack);
    }

    void initHandlers() {
        MessageService::RegisterMessageHandler(Message::MessageType_WhoAreYou, whoAreYouHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_PlayAnim, playLEDAnimHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_StopAnim, stopLEDAnimHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_StopAllAnims, stopAllLEDAnimsHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_PowerOperation, powerOperationHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_ClearSettings, clearSettingsHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_StoreValue, storeValueHandler);

        Stack::hook(onConnectionEvent, nullptr);
    }

    void initLogicHandlers() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetTopLevelState, setTopLevelStateHandler);
        ChargerProximity::hook(onChargerStateChange, nullptr);
    }
}
