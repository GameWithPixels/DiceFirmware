#include "drivers_nrf/mcu_temperature.h"
#include "drivers_nrf/scheduler.h"
#include "core/delegate_array.h"

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
    void notifyClients(void * p_event_data, uint16_t event_size);
    void getTemperatureHandler(const Message* msg);

    void init(TemperatureInitCallback callback) {
        nrfx_temp_init(&temp_config, temperatureReadyHandler);
        ret_code_t ret = nrfx_temp_measure();
        if (ret == NRF_SUCCESS) {
            // Since reading temperature takes times and uses our interrupt handler, register a lambda
            // and we'll unregister it once we have a result.
            clients.Register((void*)callback, [] (void* the_callback, int the_temp) {
                NRF_LOG_INFO("MCU Temperature Initialized, Temp = %d.%d C", (the_temp / 100), (the_temp % 100));
                clients.UnregisterWithToken(the_callback);

                // Register ourselves as handling temperature messages
                MessageService::RegisterMessageHandler(Message::MessageType_RequestTemperature, getTemperatureHandler);

                ((TemperatureInitCallback)the_callback)(true);
            });
        } else {
            callback(false);
        }
    }

    void temperatureReadyHandler(int32_t raw_measurement) {
        int celsiusTimes100 = nrfx_temp_calculate(raw_measurement);
		for (int i = 0; i < clients.Count(); ++i) {
			clients[i].handler(clients[i].token, celsiusTimes100);
		}
    }

    void getTemperatureHandler(const Message* msg) {
        // Since reading temperature takes times and uses our interrupt handler, register a lambda
        // and we'll unregister it once we have a result.
        void* uniqueToken = (void*)0x1234;
        ret_code_t ret = nrfx_temp_measure();
        if (ret == NRF_SUCCESS) {
            clients.Register(uniqueToken, [] (void* the_uniquetoken, int the_temp) {
                NRF_LOG_INFO("Temperature Requested, Temp = %d.%d C", (the_temp / 100), (the_temp % 100));
                clients.UnregisterWithToken(the_uniquetoken);

                // Send message back
                MessageTemperature tmp;
                tmp.mcuTempTimes100 = the_temp;
                tmp.batteryTempTimes100 = (uint16_t)(NTC::getNTCTemperature() * 100.0f);
                MessageService::SendMessage(&tmp);
            });
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
	void hook(TemperatureClientMethod method, void* parameter)
	{
		if (!clients.Register(parameter, method))
		{
			NRF_LOG_ERROR("Too many Temperature hooks registered.");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHook(TemperatureClientMethod method)
	{
		clients.UnregisterWithHandler(method);
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHookWithParam(void* param)
	{
		clients.UnregisterWithToken(param);
	}

}
