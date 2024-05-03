#include "temperature.h"
#include "drivers_nrf/mcu_temperature.h"
#include "drivers_hw/ntc.h"
#include "drivers_nrf/timers.h"
#include "core/delegate_array.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"
#include "nrf_log.h"

using namespace DriversNRF;
using namespace DriversHW;
using namespace Bluetooth;

#define MAX_TEMPERATURE_CLIENTS 2
#define TEMPERATURE_TIMER_MS 1000	// ms
#define TEMPERATURE_TIMER_MS_SLOW 10000	// ms
#define MCU_TEMPERATURE_CHANGE_THRESHOLD 100 // 1 degree
#define NTC_TEMPERATURE_CHANGE_THRESHOLD 10 // 0.1 degree

#define MCU_TEMPERATURE_LOW_THRESHOLD (-100) //-1 degree
#define MCU_TEMPERATURE_HIGH_THRESHOLD 10000 //100 degree
#define NTC_TEMPERATURE_LOW_THRESHOLD (-100) //-1 degree
#define NTC_TEMPERATURE_HIGH_THRESHOLD 10000 //100 degree


namespace Modules::Temperature
{
    static int32_t currentMCUTemperature;
    static int32_t currentNTCTemperature;
    static uint16_t temperatureTimerMs = TEMPERATURE_TIMER_MS;
    APP_TIMER_DEF(temperatureTimer);

    DelegateArray<TemperatureChangeClientMethod, MAX_TEMPERATURE_CLIENTS> clients;

    void getTemperatureHandler(const Message* msg);
    void update(void* context);

    static InitCallback the_callback = nullptr;
    void init(InitCallback callback) {
        the_callback = callback;
        // Measure initial temperatures
        if (!NTC::measure([](int32_t ntcTimes100) {
            currentNTCTemperature = ntcTimes100;
            currentMCUTemperature = MCUTemperature::measure();

            MessageService::RegisterMessageHandler(Message::MessageType_RequestTemperature, getTemperatureHandler);

            Timers::createTimer(&temperatureTimer, APP_TIMER_MODE_SINGLE_SHOT, update);
            Timers::startTimer(temperatureTimer, APP_TIMER_TICKS(temperatureTimerMs), NULL);

            // Check that the measured voltages are in a valid range
            bool success = currentMCUTemperature > MCU_TEMPERATURE_LOW_THRESHOLD && currentMCUTemperature < MCU_TEMPERATURE_HIGH_THRESHOLD &&
                            currentNTCTemperature > NTC_TEMPERATURE_LOW_THRESHOLD && currentNTCTemperature < NTC_TEMPERATURE_HIGH_THRESHOLD;
            if (!success) {
                NRF_LOG_ERROR("Temperature invalid: MCU: %d.%02d, Batt: %d.%02d", currentMCUTemperature / 100, currentMCUTemperature % 100, currentNTCTemperature / 100, currentNTCTemperature % 100);
            } else {
                NRF_LOG_INFO("Temperature initialized: MCU: %d.%02d, Batt: %d.%02d", currentMCUTemperature / 100, currentMCUTemperature % 100, currentNTCTemperature / 100, currentNTCTemperature % 100);
            }

            the_callback(success);
        })) {
            // Send error message back
            the_callback(false);
        }
    }

    void getTemperatureHandler(const Message* msg) {
           
        // Since reading temperature takes times and uses our interrupt handler, register a lambda
        // and we'll unregister it once we have a result.
        // Send message back
        MessageTemperature tmp;
        tmp.mcuTempTimes100 = currentMCUTemperature;
        tmp.batteryTempTimes100 = currentNTCTemperature;
        MessageService::SendMessage(&tmp);
    }

    static int32_t newMCUTemperature;
    static int32_t newNTCTemperature;
    void update(void* context) {
        // Measure new temperatures
        if (!NTC::measure([](int32_t ntcTimes100) {
            newNTCTemperature = ntcTimes100;
            newMCUTemperature = MCUTemperature::measure();
            // Did the temperatures change by more than 1 degree?
            if ((newMCUTemperature <= (currentMCUTemperature - MCU_TEMPERATURE_CHANGE_THRESHOLD)) || (newMCUTemperature >= (currentMCUTemperature + MCU_TEMPERATURE_CHANGE_THRESHOLD)) ||
                (newNTCTemperature <= (currentNTCTemperature - NTC_TEMPERATURE_CHANGE_THRESHOLD)) || (newNTCTemperature >= (currentNTCTemperature + NTC_TEMPERATURE_CHANGE_THRESHOLD))) {

                // Update temperatures
                currentMCUTemperature = newMCUTemperature;
                currentNTCTemperature = newNTCTemperature;
                // Notify clients
                for (int i = 0; i < clients.Count(); ++i) {
                    clients[i].handler(clients[i].token, currentMCUTemperature, currentNTCTemperature);
                }
            }

            // Restart the timer in any case
            Timers::startTimer(temperatureTimer, APP_TIMER_TICKS(temperatureTimerMs), NULL);
        })) {
            NRF_LOG_WARNING("Unable to measure NTC temperature");
            Timers::startTimer(temperatureTimer, APP_TIMER_TICKS(temperatureTimerMs), NULL);
        }
    }

    int16_t getMCUTemperatureTimes100() {
        return currentMCUTemperature;
    }

    int16_t getNTCTemperatureTimes100() {
        return currentNTCTemperature;
    }

    void slowMode(bool slow) {
        if (slow) {
            temperatureTimerMs = TEMPERATURE_TIMER_MS_SLOW;
        } else {
            temperatureTimerMs = TEMPERATURE_TIMER_MS;
        }
        // The new timer duration will kick in on the next reset of the battery timer.
    }

    bool hookTemperatureChange(TemperatureChangeClientMethod method, void* param) {
        return clients.Register(param, method);
    }

    void unHookTemperatureChange(TemperatureChangeClientMethod client) {
        clients.UnregisterWithHandler(client);
    }

    void unHookTemperatureChangeWithParam(void* param) {
        clients.UnregisterWithToken(param);
    }
}
