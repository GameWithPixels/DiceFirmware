#include "power_event.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/power_manager.h"
#include "nrf_log.h"

using namespace Bluetooth;
using namespace Modules;
using namespace DriversNRF;

namespace Handlers::PowerEvent
{
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

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_PowerOperation, powerOperationHandler);
    }
}