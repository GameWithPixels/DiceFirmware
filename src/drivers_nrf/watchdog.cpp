#include "watchdog.h"
#include "nrf_drv_wdt.h"
#include "power_manager.h"
#include "timers.h"
#include "drivers_nrf/log.h"

static nrf_drv_wdt_channel_id m_channel_id;

//#define RESET_FLAG_TIME_MS 3000
#define RESET_FLAG_TIME_MS 30000

namespace DriversNRF::Watchdog
{
    // WDT events handler.
    static void wdt_event_handler(void)
    {
        NRF_LOG_ERROR("Watchdog Resetting Die");
        Log::process();
    }

    // Initialize the watchdog
    void init()
    {
        //Configure WDT.
        nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
        config.behaviour = NRF_WDT_BEHAVIOUR_PAUSE_SLEEP_HALT;
        ret_code_t err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
        APP_ERROR_CHECK(err_code);

        // Returns no error code to check
        nrf_drv_wdt_enable();
    }

    void feed()
    {
        nrf_drv_wdt_channel_feed(m_channel_id);
    }

    void selfTest()
    {

    }
}
