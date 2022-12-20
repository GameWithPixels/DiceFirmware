#include "roll.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "modules/accelerometer.h"
#include "nrf_log.h"

using namespace Bluetooth;
using namespace Modules;

namespace Notifications::Roll
{
    void requestRollStateHandler(const Message *message);
    void onRollStateChange(void *token, Accelerometer::RollState newRollState, int newFace);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_RequestRollState, requestRollStateHandler);
        Accelerometer::hookRollState(onRollStateChange, nullptr);

        NRF_LOG_DEBUG("Roll notifications init");
    }

    void sendRollState(Accelerometer::RollState rollState, int face) {
        if (MessageService::canSend()) {
            NRF_LOG_INFO("Sending roll state: %d, face: %d", rollState, face);
            MessageRollState rollStateMsg;
            rollStateMsg.state = (uint8_t)rollState;
            rollStateMsg.face = (uint8_t)face;
            MessageService::SendMessage(&rollStateMsg);
        }
    }

    void requestRollStateHandler(const Message* message) {
        NRF_LOG_DEBUG("Received Roll State Request");
        sendRollState(Accelerometer::currentRollState(), Accelerometer::currentFace());
    }

    void onRollStateChange(void* token, Accelerometer::RollState newRollState, int newFace) {
        sendRollState(newRollState, newFace);
    }
}
