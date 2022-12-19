#include "drivers_nrf/mcu_temperature.h"
#include "drivers_nrf/scheduler.h"
#include "core/delegate_array.h"
#include "app_error.h"

#include "nrfx_temp.h"
#include "nrf_log.h"
#include "nrf_error.h"

#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_stack.h"

#include "drivers_hw/ntc.h"

using namespace Bluetooth;
using namespace DriversHW;

namespace DriversNRF::MCUTemperature
{
    const nrfx_temp_config_t temp_config = NRFX_TEMP_DEFAULT_CONFIG;

    #define MAX_CLIENTS 2
	DelegateArray<TemperatureClientMethod, MAX_CLIENTS> clients;

    void temperatureReadyHandler(int32_t raw_measurement);
    void getTemperatureHandler(const Message* msg);

    void init() {
        ret_code_t err_code = nrfx_temp_init(&temp_config, temperatureReadyHandler);
        APP_ERROR_CHECK(err_code);

        // Register ourselves as handling temperature messages
        MessageService::RegisterMessageHandler(Message::MessageType_RequestTemperature, getTemperatureHandler);

        NRF_LOG_INFO("Temperature Initialized");
    }

    bool measure() {
        ret_code_t ret = nrfx_temp_measure();
        return ret == NRF_SUCCESS;
    }

    void temperatureReadyHandler(int32_t raw_measurement) {
        int celsiusTimes100 = nrfx_temp_calculate(raw_measurement);
        Scheduler::push(&celsiusTimes100, sizeof(int), [](void *tempTimes100Ptr, uint16_t event_size) {
            int the_tempTimes100 = *(int*)tempTimes100Ptr;
            for (int i = 0; i < clients.Count(); ++i) {
                clients[i].handler(clients[i].token, the_tempTimes100);
            }
        });
    }

    void getTemperatureHandler(const Message* msg) {
        // We don't want to register to the clients list multiple times.
        // It would happen when we get several Temp request messages before 
        // having the time to send the response.
        // In this case sending just one response for all the pending requests
        // is fine.
        static bool hasPendingRequest = false;
        if (hasPendingRequest)
            return;
            
        // Since reading temperature takes times and uses our interrupt handler, register a lambda
        // and we'll unregister it once we have a result.
        NRF_LOG_INFO("Received Temp Request");
        void* uniqueToken = (void*)0x1234;
        ret_code_t ret = nrfx_temp_measure();
        if (ret == NRF_SUCCESS) {
            if (clients.Register(uniqueToken, [] (void* the_uniquetoken, int the_tempTimes100) {
                NRF_LOG_INFO("Sending temp: %d.%d C", (the_tempTimes100 / 100), (the_tempTimes100 % 100));
                clients.UnregisterWithToken(the_uniquetoken);

                // Send message back
                MessageTemperature tmp;
                tmp.mcuTempTimes100 = the_tempTimes100;
                tmp.batteryTempTimes100 = (uint16_t)(NTC::getNTCTemperature() * 100.0f);
                MessageService::SendMessage(&tmp);

                // The response message was send, allow ourselves to register
                // to the clients list again on the next temp request
                hasPendingRequest = false;
            })) {
                hasPendingRequest = true;
            }
        } else {
            // Send message back
            MessageTemperature tmp;
            tmp.mcuTempTimes100 = 0xFFFF;
            tmp.batteryTempTimes100 = (uint16_t)(NTC::getNTCTemperature() * 100.0f);
            MessageService::SendMessage(&tmp);
        }
    }

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hook(TemperatureClientMethod method, void* parameter) {
		if (!clients.Register(parameter, method))
		{
			NRF_LOG_ERROR("Too many Temperature hooks registered.");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHook(TemperatureClientMethod method) {
		clients.UnregisterWithHandler(method);
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHookWithParam(void* param) {
		clients.UnregisterWithToken(param);
	}
}
