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
#include "modules/temperature.h"
#include "drivers_nrf/mcu_temperature.h"
#include "drivers_hw/ntc.h"
#include "drivers_hw/battery.h"
#include "utils/utils.h"
#include "config/board_config.h"
#include "modules/leds.h"

using namespace Modules;
using namespace Bluetooth;
using namespace Utils;
using namespace Config;

namespace Bluetooth::Telemetry
{
    static MessageTelemetry teleMessage;
    static TelemetryRequestMode requestMode = TelemetryRequestMode_Off;
    static uint32_t minIntervalMs = 0;
    static uint32_t lastMessageMs = 0;

    void onRequestTelemetryHandler(const Message* message);

    void init() {
        // Init our reuseable telemetry message
        memset(&teleMessage, 0, sizeof(teleMessage));
        teleMessage.type = Message::MessageType_Telemetry;

        // Register for messages to send telemetry data over!
        MessageService::RegisterMessageHandler(Message::MessageType_RequestTelemetry, onRequestTelemetryHandler);

        NRF_LOG_DEBUG("Telemetry init");
    }

    void onConnectionEvent(void* param, bool connected) {
        if (!connected) {
            stop();
        }
    }

    void trySend() {
        // Check that we got the acceleration, RSSI and temperature data
        const bool allInit = teleMessage.time != 0 && teleMessage.rssi;

        // Check time interval since we last send a telemetry message
        const uint32_t time = DriversNRF::Timers::millis();
        if (allInit && time - lastMessageMs >= minIntervalMs) {
            if (MessageService::isConnected()) {
                lastMessageMs = time;
                if (requestMode == TelemetryRequestMode_Once) {
                    stop();
                }

                // Update battery values
                teleMessage.internalChargeState = DriversHW::Battery::checkCharging() ? 1 : 0;
                teleMessage.batteryControllerMode = BatteryController::getControllerOverrideMode();
                teleMessage.batteryLevelPercent = BatteryController::getLevelPercent();
                teleMessage.batteryState = BatteryController::getBatteryState();
                teleMessage.batteryControllerState = BatteryController::getState();

                // Voltage and current
                teleMessage.voltageTimes50 = BatteryController::getVoltageMilli() / 20;
                teleMessage.vCoilTimes50 = BatteryController::getCoilVoltageMilli() / 20;
                teleMessage.ledCurrent = LEDs::computeCurrentEstimate();

                // Send the message
                NRF_LOG_DEBUG("Sending telemetry: %d", teleMessage.time);
                MessageService::SendMessage(&teleMessage);
            } else {
                NRF_LOG_DEBUG("Disconnected, skipped sending telemetry message");
            }
        }
    }

    void onAccDataReceived(void* param, const Accelerometer::AccelFrame& frame) {
        teleMessage.acc = frame.acc;
        teleMessage.faceConfidenceTimes1000 = frame.faceConfidenceTimes1000;
        teleMessage.time = frame.time;
        teleMessage.rollState = frame.rollState;
        teleMessage.face = frame.face;
        trySend();
    }

    void onRssiChanged(void* param, int8_t rssi, uint8_t channelIndex) {
        teleMessage.rssi = rssi;
        teleMessage.channelIndex = channelIndex;
        trySend();
    }

    void onTemperatureChanged(void* param, int32_t mcuTemperatureTimes100, int32_t ntcTemperatureTimes100) {
       if (teleMessage.mcuTempTimes100 != mcuTemperatureTimes100 ||
            teleMessage.batteryTempTimes100 != ntcTemperatureTimes100) {
            teleMessage.mcuTempTimes100 = mcuTemperatureTimes100;
            teleMessage.batteryTempTimes100 = ntcTemperatureTimes100;
            trySend();
        }
    }

    void onBatteryChanged(void* param, BatteryController::State newState) {
        trySend();
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
            NRF_LOG_INFO("Telemetry on @ %dms", minInterval);
            requestMode = repeat ? TelemetryRequestMode_Repeat : TelemetryRequestMode_Once;

            // Reset accel data and RSSI so we won't send a message until they are updated
            teleMessage.time = 0;
            teleMessage.rssi = 0;

            // Update temperature to have some valid values until the temp callback is called
            teleMessage.mcuTempTimes100 = Temperature::getMCUTemperatureTimes100();
            teleMessage.batteryTempTimes100 = Temperature::getNTCTemperatureTimes100();

            // Monitor connections status
            Bluetooth::Stack::hook(onConnectionEvent, nullptr);

            // Ask the acceleration controller to be notified when
            // new acceleration data comes in!
            Accelerometer::hookFrameData(onAccDataReceived, nullptr);

            // RSSI
            Stack::hookRssi(onRssiChanged, nullptr);

            // Temperature changes
            Temperature::hookTemperatureChange(onTemperatureChanged, nullptr);

            // Battery controller state
            Modules::BatteryController::hookControllerState(onBatteryChanged, nullptr);
            Modules::BatteryController::setUpdateRate(BatteryController::UpdateRate_Fast);
        }
    }

    void stop() {
        if (requestMode != TelemetryRequestMode_Off) {
            NRF_LOG_INFO("Telemetry off");
            requestMode = TelemetryRequestMode_Off;

            // Stop being notified!
            Bluetooth::Stack::unHook(onConnectionEvent);
            Accelerometer::unHookFrameData(onAccDataReceived);
            Stack::unHookRssi(onRssiChanged);
            Temperature::unHookTemperatureChange(onTemperatureChanged);
            Modules::BatteryController::setUpdateRate(BatteryController::UpdateRate_Normal);
            Modules::BatteryController::unHookControllerState(onBatteryChanged);
        }
    }
}
