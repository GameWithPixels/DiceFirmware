#include "battery.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "modules/battery_controller.h"
#include "nrf_log.h"

using namespace Bluetooth;
using namespace Modules;

namespace Notifications::Battery
{
    void requestBatteryLevelHandler(const Message *message);
    void onBatteryStateChange(void *token, BatteryController::BatteryState newState);
    void onBatteryLevelChange(void *param, uint8_t levelPercent);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_RequestBatteryLevel, requestBatteryLevelHandler);
        BatteryController::hookBatteryState(onBatteryStateChange, nullptr);
        BatteryController::hookLevel(onBatteryLevelChange, nullptr);

        NRF_LOG_DEBUG("Battery notifications init");
    }

    void notifyConnectionEvent(bool connected) {
    }

    void sendBatteryLevel() {
        if (MessageService::canSend()) {
            const auto level = BatteryController::getLevelPercent();
            const auto state = BatteryController::getBatteryState();
            NRF_LOG_INFO("Sending battery level: %d%%, state: %d", level, state);
            MessageBatteryLevel batteryMsg;
            batteryMsg.levelPercent = level;
            batteryMsg.state = state;
            MessageService::SendMessage(&batteryMsg);
        }
    }

    void requestBatteryLevelHandler(const Message* message) {
        NRF_LOG_INFO("Received Battery Level Request");
        sendBatteryLevel();
    }

    void onBatteryLevelChange(void *param, uint8_t levelPercent) {
        sendBatteryLevel();
    }

    void onBatteryStateChange(void* token, BatteryController::BatteryState newState) {
        sendBatteryLevel();
    }
}
