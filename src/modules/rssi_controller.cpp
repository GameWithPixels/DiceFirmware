#include "rssi_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "nrf_log.h"

using namespace Bluetooth;

namespace Modules
{
namespace RssiController
{
    void GetRssiHandler(const Message* msg);
    void OnRssi(void *token, int8_t rssi, uint8_t channelIndex);

    void init() {
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_RequestRssi, GetRssiHandler);
        NRF_LOG_INFO("Rssi controller initialized");
    }

    void GetRssiHandler(const Message* msg) {
        NRF_LOG_INFO("Rssi requested");
        Stack::hookRssi(OnRssi, nullptr);
    }

    void OnRssi(void* token, int8_t rssi, uint8_t channelIndex) {
        NRF_LOG_INFO("Returning Rssi: %d", rssi);
        Stack::unHookRssi(OnRssi);
        MessageRssi retMsg;
        retMsg.rssi = rssi;
        retMsg.channelIndex = channelIndex;
        MessageService::SendMessage(&retMsg);
    }
}
}