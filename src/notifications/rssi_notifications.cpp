#include "rssi.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_nrf/timers.h"
#include "nrf_log.h"

using namespace Bluetooth;

namespace Notifications::Rssi
{
    static TelemetryRequestMode requestMode = TelemetryRequestMode_Off;
    static int8_t lastRssi = 0;
    static uint32_t lastMessageMs = 0;
    static uint32_t minIntervalMs = 0;

    void getRssiHandler(const Message *msg);
    void stop();
    void onRssi(void *token, int8_t rssi, uint8_t channelIndex);

    void init() {
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_RequestRssi, getRssiHandler);
        NRF_LOG_INFO("Rssi notifications initialized");
    }

    void notifyConnectionEvent(bool connected) {
        if (!connected) {
            stop();
        }
    }
    
    void getRssiHandler(const Message* msg) {
        auto reqRssi = static_cast<const MessageRequestRssi *>(msg);
        NRF_LOG_INFO("Received RSSI Request, mode = %d", reqRssi->requestMode);

        if (reqRssi->requestMode != TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Starting sending RSSI");
            if (requestMode == TelemetryRequestMode_Off) {
                Stack::hookRssi(onRssi, nullptr);
            }
            requestMode = reqRssi->requestMode;
            minIntervalMs = reqRssi->minInterval;
            // Reset timestamp so next message is send on the first RSSI update
            lastMessageMs = 0;
        }
        else if (requestMode == TelemetryRequestMode_Repeat) {
            // If requestMode is "once", it will auto turn off on
            // the next RSSI update
            stop();
        }
    }

    void stop() {
        if (requestMode != TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Stopping sending RSSI");
            requestMode = TelemetryRequestMode_Off;
            Stack::unHookRssi(onRssi);
        }
    }

    void onRssi(void* token, int8_t rssi, uint8_t channelIndex) {
        // Check time interval since we last send a telemetry message
        const uint32_t time = DriversNRF::Timers::millis();
        if (lastMessageMs == 0 ||
            (time - lastMessageMs >= minIntervalMs && rssi != lastRssi)) {
            if (MessageService::canSend()) {
                lastMessageMs = time;
                lastRssi = rssi;
                if (requestMode == TelemetryRequestMode_Once) {
                    // Send RSSI only once
                    stop();
                }

                // Send the message
                NRF_LOG_INFO("Sending RSSI: %d", rssi);
                MessageRssi retMsg;
                retMsg.rssi = rssi;
                MessageService::SendMessage(&retMsg);
            } else {
                NRF_LOG_DEBUG("Couldn't send RSSI message");
            }
        }
    }
}
