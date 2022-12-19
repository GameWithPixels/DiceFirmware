#include "telemetry.h"
#include "bluetooth_message_service.h"
#include "bluetooth_messages.h"
#include "bluetooth_stack.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_log.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "drivers_nrf/timers.h"
#include "modules/accelerometer.h"
#include "modules/battery_controller.h"
#include "modules/rssi_controller.h"
#include "drivers_nrf/mcu_temperature.h"
#include "utils/utils.h"

using namespace Modules;
using namespace Bluetooth;
using namespace Utils;

namespace Bluetooth::Telemetry
{
    static MessageTelemetry teleMessage;
    static TelemetryRequestMode requestMode = TelemetryRequestMode_Off;
    static uint32_t minIntervalMs = 0;
    static uint32_t lastMessageMs = 0;

    void onConnectionEvent(void* param, bool connected);
    void onRequestTelemetryHandler(const Message* message);

    void init() {
        // Init our reuseable telemetry message
        memset(&teleMessage, 0, sizeof(teleMessage));
        teleMessage.type = Message::MessageType_Telemetry;

        // Register for messages to send telemetry data over!
        MessageService::RegisterMessageHandler(Message::MessageType_RequestTelemetry, onRequestTelemetryHandler);
        Bluetooth::Stack::hook(onConnectionEvent, nullptr);

        NRF_LOG_INFO("Telemetry initialized");
    }

    void onConnectionEvent(void* param, bool connected) {
        if (!connected) {
            stop();
        }
    }

    void update() {
        uint32_t time = DriversNRF::Timers::millis();
        if (time - lastMessageMs >= minIntervalMs) {
            if (MessageService::canSend()) {
                // Update state
                lastMessageMs = time;
                if (requestMode == TelemetryRequestMode_Once) {
                    stop();
                }

                // Send the message
                NRF_LOG_INFO("Sending telemetry");
                MessageService::SendMessage(&teleMessage);
            }
            else {
                NRF_LOG_DEBUG("Couldn't send telemetry message");
            }
        }
    }

    void onAccDataReceived(void* param, const Accelerometer::AccelFrame& frame) {
        teleMessage.accelFrame = frame;
        update();
    }

    void onBatteryStateChanged(void* param, BatteryController::BatteryState chargeState) {
        teleMessage.batteryChargeState = chargeState;
        update();
    }

    void onBatteryLevelChanged(void* param, uint8_t levelPercent) {
        teleMessage.batteryLevelPercent = levelPercent;
        update();
    }

    void onRssiChanged(void* param, int8_t rssi, uint8_t channelIndex) {
        teleMessage.rssi = rssi;
        teleMessage.channelIndex = channelIndex;
        update();
    }

    void onTemperatureRead(void* param, int temperatureTimes100) {
        if (teleMessage.tempTimes100 != temperatureTimes100) {
            teleMessage.tempTimes100 = temperatureTimes100;
            update();
        }
    }

    void onRequestTelemetryHandler(const Message* message) {
        auto reqTelem = static_cast<const MessageRequestTelemetry *>(message);
        NRF_LOG_INFO("Received Telemetry Request, mode = %d, minInterval = %d", reqTelem->requestMode, reqTelem->minInterval);
        if (reqTelem->requestMode != TelemetryRequestMode_Off) {
            start(reqTelem->requestMode == TelemetryRequestMode_Repeat, reqTelem->minInterval);
        }
        else {
            stop();
        }
    }

    void start(bool repeat, uint32_t minInterval) {
        minIntervalMs = minInterval;
        lastMessageMs = 0; // Reset timestamp so next message is send ASAP

        if (requestMode == TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Starting Telemetry");
            requestMode = repeat ? TelemetryRequestMode_Repeat : TelemetryRequestMode_Once;

            // Ask the acceleration controller to be notified when
            // new acceleration data comes in!
            Accelerometer::hookFrameData(onAccDataReceived, nullptr);

            // Battery
            BatteryController::hook(onBatteryStateChanged, nullptr);
            BatteryController::hookLevel(onBatteryLevelChanged, nullptr);

            // RSSI
            Stack::hookRssi(onRssiChanged, nullptr);

            // Temperature
            DriversNRF::MCUTemperature::hook(onTemperatureRead, nullptr);
        }
    }

    void stop() {
        if (requestMode != TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Stopping Telemetry");
            requestMode = TelemetryRequestMode_Off;

            // Stop being notified!
            Accelerometer::unHookFrameData(onAccDataReceived);

            BatteryController::unHook(onBatteryStateChanged);
            BatteryController::unHookLevel(onBatteryLevelChanged);

            Stack::unHookRssi(onRssiChanged);

            DriversNRF::MCUTemperature::unHook(onTemperatureRead);
        }
    }
}
