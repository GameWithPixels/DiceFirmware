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

    bool measure(TemperatureClientMethod callback, void* param) {
		bool ret = clients.Register(param, callback);
		if (ret) {
			ret = nrfx_temp_measure() == NRFX_SUCCESS;
		}
		return ret;
    }

    void temperatureReadyHandler(int32_t raw_measurement) {
        int celsiusTimes100 = nrfx_temp_calculate(raw_measurement);
        Scheduler::push(&celsiusTimes100, sizeof(int), [](void *tempTimes100Ptr, uint16_t event_size) {
			DelegateArray<TemperatureClientMethod, MAX_CLIENTS> copy;
			for (int i = 0; i < clients.Count(); ++i) {
				copy.Register(clients[i].token, clients[i].handler);
			}
			clients.UnregisterAll();
            int the_tempTimes100 = *(int*)tempTimes100Ptr;
            for (int i = 0; i < copy.Count(); ++i) {
                copy[i].handler(copy[i].token, the_tempTimes100);
            }
        });
    }
}
