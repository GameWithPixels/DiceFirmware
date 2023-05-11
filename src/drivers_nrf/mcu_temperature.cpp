#include "drivers_nrf/mcu_temperature.h"
#include "drivers_nrf/scheduler.h"
#include "app_error.h"

#include "nrfx_temp.h"
#include "nrf_log.h"
#include "nrf_error.h"

namespace DriversNRF::MCUTemperature
{
    const nrfx_temp_config_t temp_config = NRFX_TEMP_DEFAULT_CONFIG;

    void init() {
        ret_code_t err_code = nrfx_temp_init(&temp_config, nullptr);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("Temperature init");
    }

    int32_t measure() {
		if (nrfx_temp_measure() == NRFX_SUCCESS) {
			uint32_t raw_temp = nrfx_temp_result_get();
        	return nrfx_temp_calculate(raw_temp);
		} else {
			return 0;
		}
    }
}
