#include "drivers_nrf/mcu_temperature.h"
#include "drivers_nrf/scheduler.h"
#include "core/delegate_array.h"
#include "app_error.h"

#include "nrfx_temp.h"
#include "nrf_log.h"
#include "nrf_error.h"

namespace DriversNRF::MCUTemperature
{
    const nrfx_temp_config_t temp_config = NRFX_TEMP_DEFAULT_CONFIG;

    #define MAX_CLIENTS 2
	DelegateArray<TemperatureClientMethod, MAX_CLIENTS> clients;

    void temperatureReadyHandler(int32_t raw_measurement);

    void init() {
        ret_code_t err_code = nrfx_temp_init(&temp_config, temperatureReadyHandler);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("Temperature init");
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
