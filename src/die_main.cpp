#include "die.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/log.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/battery_controller.h"
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

    void RequestStateHandler(void* token, const Message* message);
    void WhoAreYouHandler(void* token, const Message* message);
    void onBatteryStateChange(void* token, BatteryController::BatteryState newState);
    void onRollStateChange(void* token, Accelerometer::RollState newRollState, int newFace);
    void SendRollState(Accelerometer::RollState rollState, int face);
    void PlayLEDAnim(void* context, const Message* msg);
    void StopLEDAnim(void* context, const Message* msg);
    void StopAllLEDAnims(void* context, const Message* msg);
    void SetTopLevelState(void* context, const Message* msg);
	void EnterStandardState();
	void EnterLEDAnimState();
    void EnterSleepMode(void* token, const Message* message);
    void onConnection(void* token, bool connected);

    void initMainLogic() 
    {
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_WhoAreYou, nullptr, WhoAreYouHandler);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_PlayAnim, nullptr, PlayLEDAnim);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_StopAnim, nullptr, StopLEDAnim);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_StopAllAnims, nullptr, StopAllLEDAnims);
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_Sleep, nullptr, EnterSleepMode);
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_RequestRollState, nullptr, RequestStateHandler);

        NRF_LOG_INFO("Main Logic Initialized");
    }

    void initDieLogic() 
    {
        Bluetooth::MessageService::RegisterMessageHandler(Message::MessageType_SetTopLevelState, nullptr, SetTopLevelState);
        
        Bluetooth::Stack::hook(onConnection, nullptr);

        BatteryController::hook(onBatteryStateChange, nullptr);

        Accelerometer::hookRollState(onRollStateChange, nullptr);

        currentFace = Accelerometer::currentFace();

        NRF_LOG_INFO("Die Logic Initialized");
    }

    void RequestStateHandler(void* token, const Message* message) {
        SendRollState(Accelerometer::currentRollState(), Accelerometer::currentFace());
    }

    void SendRollState(Accelerometer::RollState rollState, int face) {
        // Central asked for the die state, return it!
        Bluetooth::MessageRollState rollStateMsg;
        rollStateMsg.state = (uint8_t)rollState;
        rollStateMsg.face = (uint8_t)face;
        Bluetooth::MessageService::SendMessage(&rollStateMsg);
    }

    void WhoAreYouHandler(void* token, const Message* message) {
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

    void onConnection(void* token, bool connected) {
        if (connected) {
            // Nothing
        } else {
            // Return to solo play
            EnterStandardState();
        }
    }

    void PlayLEDAnim(void* context, const Message* msg) {
        auto playAnimMessage = (const MessagePlayAnim*)msg;
        NRF_LOG_INFO("Playing animation %d", playAnimMessage->animation);
        AnimController::play(playAnimMessage->animation, playAnimMessage->remapFace, playAnimMessage->loop);
    }

    void StopLEDAnim(void* context, const Message* msg) {
        auto stopAnimMessage = (const MessageStopAnim*)msg;
        NRF_LOG_INFO("Stopping animation %d", stopAnimMessage->animation);
        AnimController::stop((int)stopAnimMessage->animation, stopAnimMessage->remapFace);
    }

    void StopAllLEDAnims(void* context, const Message* msg) {
        NRF_LOG_INFO("Stopping all animations");
        AnimController::stopAll();
    }

    void SetTopLevelState(void *context, const Message *msg) {
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
    
    void EnterSleepMode(void* token, const Message* msg) {
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