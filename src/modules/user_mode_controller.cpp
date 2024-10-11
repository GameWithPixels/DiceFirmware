#include "user_mode_controller.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "nrf_log.h"
#include "die.h"
#include "modules/behavior_controller.h"

using namespace Bluetooth;
using namespace Modules;

namespace Modules::UserModeController
{
    UserMode currentUserMode;
    void setUserModeHandler(const Message *msg) {

        static auto exitRemoteControl = []() {
            // Unhook connection event callback
            Bluetooth::Stack::unHookWithParam((void*)0x8E8073); // No meaning, just a token to make unhooking easy.

            // Return the die to its standard state
            BehaviorController::EnableAccelerometerRules();
            BehaviorController::EnableBatteryRules();

            // Reset user mode
            currentUserMode = UserMode_Default;
        };

        auto setUserModeMsg = (const MessageSetUserMode*)msg;
        if (setUserModeMsg->userMode == UserMode_RemoteControlled) {
            BehaviorController::DisableBatteryRules();
            BehaviorController::DisableAccelerometerRules();

            // Just to be safe, add disconnection handler to reset the state
            Bluetooth::Stack::hook([](void* param, bool connected) {
                // On disconnect, stop discharge and unhook, etc...
                exitRemoteControl();
            }, (void*)0x8E8073); // No meaning, just a token to make unhooking easy.

            currentUserMode = UserMode_Default;
            MessageService::SendMessage(Bluetooth::Message::MessageType_SetUserModeAck);
        } else {
            // Unhook
            exitRemoteControl();
            MessageService::SendMessage(Bluetooth::Message::MessageType_SetUserModeAck);
        }
    }

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_SetUserMode, setUserModeHandler);
        currentUserMode = UserMode_Default;
    }

    UserMode getCurrentUserMode() {
        return currentUserMode;
    }
}