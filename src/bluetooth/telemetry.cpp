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
#include "drivers_nrf/mcu_temperature.h"
#include "drivers_hw/ntc.h"
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

        NRF_LOG_INFO("Telemetry initialized");
    }

    void onConnectionEvent(void* param, bool connected) {
        if (!connected) {
            stop();
        }
    }

    void trySend() {
        // Check that we got the acceleration, RSSI and temperature data
        const bool allInit = teleMessage.accelFrame.time != 0 && teleMessage.rssi != 0 && teleMessage.mcuTempTimes100 != 0;

        // Check time interval since we last send a telemetry message
        const uint32_t time = DriversNRF::Timers::millis();
        if (allInit && time - lastMessageMs >= minIntervalMs) {
            if (MessageService::canSend()) {
                lastMessageMs = time;
                if (requestMode == TelemetryRequestMode_Once) {
                    stop();
                } else {
                    // Request temperature update,
                    // the result will be used on the next message
                    DriversNRF::MCUTemperature::measure();
                }

                // Update battery values
                teleMessage.batteryLevelPercent = BatteryController::getLevelPercent();
                teleMessage.batteryChargeState = BatteryController::getBatteryState();
                teleMessage.voltageTimes50 = BatteryController::getVoltageMilli() / 20;
                teleMessage.vCoilTimes50 = BatteryController::getCoilVoltageMilli() / 20;

                // Send the message
                NRF_LOG_DEBUG("Sending telemetry");
                MessageService::SendMessage(&teleMessage);
            } else {
                NRF_LOG_DEBUG("Couldn't send telemetry message");
            }
        }
    }

    void onAccDataReceived(void* param, const Accelerometer::AccelFrame& frame) {
        teleMessage.accelFrame = frame;
        trySend();
    }

    void onRssiChanged(void* param, int8_t rssi, uint8_t channelIndex) {
        teleMessage.rssi = rssi;
        teleMessage.channelIndex = channelIndex;
        trySend();
    }

    void onTemperatureRead(void* param, int temperatureTimes100) {
        auto batteryTempTimes100 = int16_t(DriversHW::NTC::getNTCTemperature() * 100.0f);
        if (teleMessage.mcuTempTimes100 != temperatureTimes100 ||
            teleMessage.batteryTempTimes100 != batteryTempTimes100) {
            teleMessage.mcuTempTimes100 = temperatureTimes100;
            teleMessage.batteryTempTimes100 = batteryTempTimes100;
            trySend();
        }
    }

    void onRequestTelemetryHandler(const Message* message) {
        auto reqTelem = static_cast<const MessageRequestTelemetry *>(message);
        NRF_LOG_DEBUG("Received Telemetry Request, mode = %d, minInterval = %d", reqTelem->requestMode, reqTelem->minInterval);
        if (reqTelem->requestMode != TelemetryRequestMode_Off) {
            start(reqTelem->requestMode == TelemetryRequestMode_Repeat, reqTelem->minInterval);
        } else {
            stop();
        }
    }

    void start(bool repeat, uint32_t minInterval) {
        minIntervalMs = minInterval;

        // Reset timestamp so next message is send on the first call to trySend()
        lastMessageMs = 0;

        if (requestMode == TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Starting Telemetry");
            requestMode = repeat ? TelemetryRequestMode_Repeat : TelemetryRequestMode_Once;

            // Reset accel data, RSSI and temperature so we won't send
            // a message until they are all updated
            teleMessage.accelFrame.time = 0;
            teleMessage.rssi = 0;
            teleMessage.mcuTempTimes100 = 0;

            // Monitor connections status
            Bluetooth::Stack::hook(onConnectionEvent, nullptr);

            // Ask the acceleration controller to be notified when
            // new acceleration data comes in!
            Accelerometer::hookFrameData(onAccDataReceived, nullptr);

            // RSSI
            Stack::hookRssi(onRssiChanged, nullptr);

            // Temperature
            DriversNRF::MCUTemperature::hook(onTemperatureRead, nullptr);

            // Request temperature update
            DriversNRF::MCUTemperature::measure();
        }
    }

    void stop() {
        if (requestMode != TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Stopping Telemetry");
            requestMode = TelemetryRequestMode_Off;

            // Stop being notified!
            Bluetooth::Stack::unHook(onConnectionEvent);

            Accelerometer::unHookFrameData(onAccDataReceived);

            Stack::unHookRssi(onRssiChanged);

            DriversNRF::MCUTemperature::unHook(onTemperatureRead);
        }
    }
}
