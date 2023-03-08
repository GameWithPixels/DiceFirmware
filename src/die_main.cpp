#include "die.h"
#include "die_private.h"
#include "pixel.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/gpiote.h"
#include "drivers_nrf/mcu_temperature.h"
#include "drivers_hw/ntc.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/battery_controller.h"
#include "modules/validation_manager.h"
#include "data_set/data_set.h"
#include "notifications/battery.h"
#include "notifications/roll.h"
#include "notifications/rssi.h"

using namespace Modules;
using namespace Bluetooth;
using namespace Accelerometer;
using namespace Config;
using namespace Animations;
using namespace DriversNRF;
using namespace DriversHW;

namespace Die
{
    static TopLevelState currentTopLevelState = TopLevel_SoloPlay;

    TopLevelState getCurrentState() {
        return currentTopLevelState;
    }

    void whoAreYouHandler(const Message *message);
    void onConnectionEvent(void *token, bool connected);

    void playLEDAnimHandler(const Message* msg);
    void stopLEDAnimHandler(const Message* msg);
    void stopAllLEDAnimsHandler(const Message* msg);
    void setTopLevelStateHandler(const Message* msg);
    void enterSleepModeHandler(const Message* message);
    void getTemperatureHandler(const Message* msg);

    void initMainLogic() {
        MessageService::RegisterMessageHandler(Message::MessageType_WhoAreYou, whoAreYouHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_PlayAnim, playLEDAnimHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_StopAnim, stopLEDAnimHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_StopAllAnims, stopAllLEDAnimsHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_Sleep, enterSleepModeHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_RequestTemperature, getTemperatureHandler);

        Stack::hook(onConnectionEvent, nullptr);

        Notifications::Battery::init();
        Notifications::Roll::init();
        Notifications::Rssi::init();

        NRF_LOG_DEBUG("Main Logic init");
    }

    void initDieLogic() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetTopLevelState, setTopLevelStateHandler);

        NRF_LOG_DEBUG("Die Logic init");
    }

    void whoAreYouHandler(const Message* message) {
        // Central asked for the die state, return it!
        Bluetooth::MessageIAmADie identityMessage;
        identityMessage.ledCount = (uint8_t)BoardManager::getBoard()->ledCount;
        identityMessage.designAndColor = SettingsManager::getSettings()->designAndColor;
        identityMessage.dataSetHash = DataSet::dataHash();
        identityMessage.pixelId = Pixel::getDeviceID();
        identityMessage.availableFlash = DataSet::availableDataSize();
        identityMessage.buildTimestamp = Pixel::getBuildTimestamp();
        identityMessage.rollState = Accelerometer::currentRollState();
        identityMessage.rollFace = Accelerometer::currentFace();
        identityMessage.batteryLevelPercent = BatteryController::getLevelPercent();
        identityMessage.batteryState = BatteryController::getBatteryState();
        MessageService::SendMessage(&identityMessage);
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
                BatteryController::slowMode(true);
                break;
            case PowerManager::PowerManagerEvent_PrepareSleep:
                //NRF_LOG_INFO("Going to Sleep");
                Accelerometer::stop();
                AnimController::stop();
                BatteryController::slowMode(true);
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
                AnimController::start();
                BatteryController::slowMode(false);
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

    // void onBatteryStateChange(void* token, BatteryController::BatteryState newState) {
    //     switch (newState) {
    //         case BatteryController::BatteryState_Charging:
    //             AnimController::play(AnimationEvent_ChargingStart);
    //             break;
    //         case BatteryController::BatteryState_Low:
    //             AnimController::play(AnimationEvent_LowBattery);
    //             break;
    //         case BatteryController::BatteryState_Ok:
    //             AnimController::play(AnimationEvent_ChargingDone);
    //             break;
    //         default:
    //             break;
    //     }
    // }

    void playLEDAnimHandler(const Message* msg) {
        auto playAnimMessage = (const MessagePlayAnim*)msg;
        NRF_LOG_DEBUG("Playing animation %d", playAnimMessage->animation);
		auto animationPreset = DataSet::getAnimation((int)playAnimMessage->animation);
        AnimController::play(animationPreset, DataSet::getAnimationBits(), playAnimMessage->remapFace, playAnimMessage->loop);
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

    void getTemperatureHandler(const Message* msg) {
        // We don't want to register to the clients list multiple times.
        // It would happen when we get several Temp request messages before 
        // having the time to send the response.
        // In this case sending just one response for all the pending requests
        // is fine.
        static bool hasPendingRequest = false;
        if (hasPendingRequest)
            return;
            
        // Since reading temperature takes times and uses our interrupt handler, register a lambda
        // and we'll unregister it once we have a result.
        NRF_LOG_DEBUG("Received Temp Request");
        void* uniqueToken = (void*)0x1234;
        if (MCUTemperature::measure()) {
            MCUTemperature::hook([] (void* the_uniquetoken, int the_tempTimes100) {
                NRF_LOG_DEBUG("Sending temp: %d.%d C", (the_tempTimes100 / 100), (the_tempTimes100 % 100));
                MCUTemperature::unHookWithParam(the_uniquetoken);

                // Send message back
                MessageTemperature tmp;
                tmp.mcuTempTimes100 = the_tempTimes100;
                tmp.batteryTempTimes100 = NTC::getNTCTemperatureTimes100();
                MessageService::SendMessage(&tmp);

                // The response message was send, allow ourselves to register
                // to the clients list again on the next temp request
                hasPendingRequest = false;
            }, uniqueToken);
        } else {
            // Send message back
            MessageTemperature tmp;
            tmp.mcuTempTimes100 = 0xFFFF;
            tmp.batteryTempTimes100 = NTC::getNTCTemperatureTimes100();
            MessageService::SendMessage(&tmp);
        }
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

    void enterStandardState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_Animator:
                // Animator mode had turned accelerometer off, restart it now
                Accelerometer::start();
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_Testing:
                // Testing mode had anim controller off, restart it now
                AnimController::start();
                Accelerometer::start();
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_SoloPlay:
                // Nothing to do
                break;
       }
    }

	void enterLEDAnimState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_SoloPlay:
                Accelerometer::stop();
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_Testing:
                AnimController::start();
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_Animator:
                // Nothing to do
                break;
       }
    }

    void enterTestingState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            case TopLevel_SoloPlay:
                Accelerometer::stop();
                AnimController::stop();
                currentTopLevelState = TopLevel_Testing;
                break;
            case TopLevel_Animator:
                AnimController::stop();
                currentTopLevelState = TopLevel_Testing;
                break;
            default:
                currentTopLevelState = TopLevel_Testing;
                break;
            case TopLevel_Testing:
                break;
       }
    }

    
    void enterSleepModeHandler(const Message* msg) {
        PowerManager::goToDeepSleep();
    }

    // Main loop!
    void update() {

        Scheduler::update();
        Watchdog::feed();
        PowerManager::update();
        MessageService::update();
    }
}

int main() {
    Die::init();
    for (;;) {
        Die::update();

        #if NRF_LOG_DEFERRED
            Log::process();
        #endif
    }
    return 0;
}
