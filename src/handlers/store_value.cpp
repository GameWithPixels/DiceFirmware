#include "store_value.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "nrf_log.h"
#include "config/value_store.h"

using namespace Bluetooth;
using namespace Modules;
using namespace Config;

namespace Handlers::StoreValue
{
    void storeValueHandler(const Message *msg) {
        auto powerMsg = (const MessageStoreValue *)msg;

        MessageStoreValueAck ack;
        ack.result = StoreValueResult_UnknownError;
        ack.index = -1;

        if (powerMsg->value) {
            const auto result = ValueStore::writeUInt32(powerMsg->value);
            if (result >= 0) {
                ack.index = result;
                ack.result = StoreValueResult_Success;
            } else {
                switch (result) {
                case Config::ValueStore::WriteValueError_StoreFull:
                    ack.result = StoreValueResult_StoreFull;
                    break;
                case Config::ValueStore::WriteValueError_NotPermited:
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

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_StoreValue, storeValueHandler);
    }
}