#include "ppi.h"
#include "config/board_config.h"
#include "nrf_drv_ppi.h"
#include "nrf_log.h"

namespace DriversNRF::PPI
{
    void init() {
        ret_code_t err_code;

        err_code = nrf_drv_ppi_init();
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("PPI init");
    }
}
