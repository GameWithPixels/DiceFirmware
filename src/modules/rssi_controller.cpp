#include "rssi_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "drivers_nrf/timers.h"
#include "nrf_log.h"

using namespace Bluetooth;

#define MIN_INTERVAL_MS 5000

namespace Modules::RssiController
{
    static TelemetryRequestMode requestMode = TelemetryRequestMode_Off;
    static int8_t lastRssi = 0;
    static uint32_t lastMessageMs = 0;

    void getRssiHandler(const Message* msg);
    void stop();
    void onConnectionEvent(void *token, bool connected);
    void onRssi(void *token, int8_t rssi, uint8_t channelIndex);

    void init() {
        Bluetooth::MessageService::RegisterMessageHandler(Bluetooth::Message::MessageType_RequestRssi, getRssiHandler);
        Bluetooth::Stack::hook(onConnectionEvent, nullptr);
        NRF_LOG_INFO("Rssi controller initialized");
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
            lastRssi = 0; // Reset value so next message is send immediately
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

    void onConnectionEvent(void* token, bool connected) {
        if (!connected) {
            stop();
        }
    }

    void onRssi(void* token, int8_t rssi, uint8_t channelIndex) {
        const bool sendOnce = requestMode == TelemetryRequestMode_Once;
        // Check time interval since we last send a telemetry message
        const uint32_t time = DriversNRF::Timers::millis();
        if (sendOnce || lastRssi == 0 ||
            (time - lastMessageMs >= MIN_INTERVAL_MS && rssi != lastRssi)) {
            if (MessageService::canSend()) {
                lastMessageMs = time;
                lastRssi = rssi;
                if (sendOnce) {
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
