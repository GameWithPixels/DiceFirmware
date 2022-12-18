#include "die.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/gpiote.h"
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
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#if !defined(BUILD_TIMESTAMP)
    #warning Build timestamp not defined
    #define BUILD_TIMESTAMP 0
#endif

using namespace DriversNRF;
using namespace Modules;
using namespace Bluetooth;
using namespace Accelerometer;
using namespace Config;
using namespace Animations;

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("app_error_handler err_code:%d %s:%d", error_code, p_file_name, line_num);
    on_error();
}


void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}


void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}


namespace Die
{
    TopLevelState currentTopLevelState = TopLevel_SoloPlay;
    int currentFace = 0;
    RollState currentRollState = RollState_Unknown;

    TopLevelState getCurrentState()
    {
        return currentTopLevelState;
    }

	void EnterStandardState();
	void EnterLEDAnimState();

    void onBatteryStateChange(void* token, BatteryController::BatteryState newState);
    void onRollStateChange(void* token, Accelerometer::RollState newRollState, int newFace);
    void onPowerEvent(void* token, PowerManager::PowerManagerEvent event);
    void SendRollState(Accelerometer::RollState rollState, int face);

    void RequestStateHandler(const Message* message);
    void WhoAreYouHandler(const Message* message);
    void PlayLEDAnimHandler(const Message* msg);
    void StopLEDAnimHandler(const Message* msg);
    void StopAllLEDAnimsHandler(const Message* msg);
    void SetTopLevelStateHandler(const Message* msg);
    void EnterSleepModeHandler(const Message* message);

    void onConnection(void* token, bool connected);

    void initMainLogic() 
    {
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_WhoAreYou, WhoAreYouHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_PlayAnim, PlayLEDAnimHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_StopAnim, StopLEDAnimHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_StopAllAnims, StopAllLEDAnimsHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_Sleep, EnterSleepModeHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_RequestRollState, RequestStateHandler);

        NRF_LOG_INFO("Main Logic Initialized");
    }

    void initDieLogic() 
    {
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_SetTopLevelState, SetTopLevelStateHandler);
        
        Bluetooth::Stack::hook(onConnection, nullptr);

        BatteryController::hook(onBatteryStateChange, nullptr);

        Accelerometer::hookRollState(onRollStateChange, nullptr);

        PowerManager::hook(onPowerEvent, nullptr);

        currentFace = Accelerometer::currentFace();

        NRF_LOG_INFO("Die Logic Initialized");
    }

    void initValidationLogic()
    {
        PowerManager::hook(onPowerEvent, nullptr);
    }

    void RequestStateHandler(const Message* message) {
        SendRollState(Accelerometer::currentRollState(), Accelerometer::currentFace());
    }

    void SendRollState(Accelerometer::RollState rollState, int face) {
        // Central asked for the die state, return it!
        Bluetooth::MessageRollState rollStateMsg;
        rollStateMsg.state = (uint8_t)rollState;
        rollStateMsg.face = (uint8_t)face;
        Bluetooth::MessageService::SendMessage(&rollStateMsg);
    }

    void WhoAreYouHandler(const Message* message) {
        // Central asked for the die state, return it!
        Bluetooth::MessageIAmADie identityMessage;
        identityMessage.ledCount = (uint8_t)BoardManager::getBoard()->ledCount;
        identityMessage.designAndColor = SettingsManager::getSettings()->designAndColor;
        identityMessage.dataSetHash = DataSet::dataHash();
        identityMessage.pixelId = getDeviceID();
        identityMessage.availableFlash = DataSet::availableDataSize();
        identityMessage.buildTimestamp = BUILD_TIMESTAMP;
        Bluetooth::MessageService::SendMessage(&identityMessage);
    }

    void onBatteryStateChange(void* token, BatteryController::BatteryState newState) {
        // switch (newState) {
        //     case BatteryController::BatteryState_Charging:
        //         AnimController::play(AnimationEvent_ChargingStart);
        //         break;
        //     case BatteryController::BatteryState_Low:
        //         AnimController::play(AnimationEvent_LowBattery);
        //         break;
        //     case BatteryController::BatteryState_Ok:
        //         AnimController::play(AnimationEvent_ChargingDone);
        //         break;
        //     default:
        //         break;
        // }
    }

    void onRollStateChange(void* token, Accelerometer::RollState newRollState, int newFace) {
        if (Bluetooth::MessageService::isConnected()) {
            SendRollState(newRollState, newFace);
        }

        currentRollState = newRollState;
        currentFace = newFace;
    }

    void onPowerEvent(void* token, PowerManager::PowerManagerEvent event) {
        switch (event) {
            case PowerManager::PowerManagerEvent_PrepareWakeUp:
                NRF_LOG_INFO("Going to low power mode");
                Accelerometer::stop();
                Accelerometer::lowPower();
                //Bluetooth::Stack::stopAdvertising();
                break;
            case PowerManager::PowerManagerEvent_PrepareSleep:
                NRF_LOG_INFO("Going to Sleep");
                Accelerometer::stop();

                if (ValidationManager::inValidation()) {
                    // In validation mode we just go to system off mode and rely
                    // on the magnet to power cycle the chip
                    PowerManager::goToSystemOff();
                } else {
                    // Set interrupt pin to wake up power manager
                    GPIOTE::enableInterrupt(
                        BoardManager::getBoard()->accInterruptPin,
                        NRF_GPIO_PIN_NOPULL,
                        NRF_GPIOTE_POLARITY_HITOLO,
                        [](uint32_t pin, nrf_gpiote_polarity_t action) {
                            Accelerometer::clearInterrupt();
                            Scheduler::push(nullptr, 0, [](void* ignoreData, uint16_t ignoreSize) {
                                PowerManager::wakeFromSleep();
                            });
                        });
                    Accelerometer::enableInterrupt();
                    //Bluetooth::Stack::stopAdvertising();
                }
                break;
            case PowerManager::PowerManagerEvent_WakingUpFromSleep:
                NRF_LOG_INFO("Resuming from Sleep");
                Accelerometer::disableInterrupt();
                Accelerometer::start();
                //Bluetooth::Stack::startAdvertising();
                break;
            default:
                break;
        }

    }

    void onConnection(void* token, bool connected) {
        if (connected) {
            // Nothing
        } else {
            // Return to solo play
            EnterStandardState();
        }
    }

    void PlayLEDAnimHandler(const Message* msg) {
        auto playAnimMessage = (const MessagePlayAnim*)msg;
        NRF_LOG_DEBUG("Playing animation %d", playAnimMessage->animation);
        AnimController::play(playAnimMessage->animation, playAnimMessage->remapFace, playAnimMessage->loop);
    }

    void StopLEDAnimHandler(const Message* msg) {
        auto stopAnimMessage = (const MessageStopAnim*)msg;
        NRF_LOG_DEBUG("Stopping animation %d", stopAnimMessage->animation);
        AnimController::stop((int)stopAnimMessage->animation, stopAnimMessage->remapFace);
    }

    void StopAllLEDAnimsHandler(const Message* msg) {
        NRF_LOG_DEBUG("Stopping all animations");
        AnimController::stopAll();
    }

    void SetTopLevelStateHandler(const Message *msg) {
        auto setTopLevelStateMessage = (const MessageSetTopLevelState *)msg;
        switch (setTopLevelStateMessage->state) {
        case TopLevel_SoloPlay:
            EnterStandardState();
            break;
        case TopLevel_Animator:
            EnterLEDAnimState();
            break;
        }
    }

    void EnterStandardState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            case TopLevel_Animator:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_SoloPlay;
                break;
            case TopLevel_SoloPlay:
            case TopLevel_LowPower:
                // Nothing to do
                break;
       }
    }

	void EnterLEDAnimState() {
        switch (currentTopLevelState) {
            case TopLevel_Unknown:
            case TopLevel_SoloPlay:
            default:
                // Reactivate playing animations based on face
                currentTopLevelState = TopLevel_Animator;
                break;
            case TopLevel_Animator:
            case TopLevel_LowPower:
                // Nothing to do
                break;
       }
    }
    
    void EnterSleepModeHandler(const Message* msg) {
        PowerManager::goToSystemOff();
    }

	uint32_t getDeviceID()
    {
		return NRF_FICR->DEVICEID[1] ^ NRF_FICR->DEVICEID[0];
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
    for (;;)
    {
        Die::update();

        #if NRF_LOG_DEFERRED
            Log::process();
        #endif
    }
    return 0;
}