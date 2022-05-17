#include "log.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "app_timer.h"
//#include "SEGGER_RTT.h"

namespace DriversNRF
{
namespace Log
{
    void init() {
        ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
        APP_ERROR_CHECK(err_code);

        // No error code to check
        NRF_LOG_DEFAULT_BACKENDS_INIT();

        NRF_LOG_INFO("----- Initializing -----");
        NRF_LOG_INFO("Log initialized");

        // Not needed for validation
        #if DICE_SELFTEST && LOG_SELFTEST
        selfTest();
        #endif

    }

    bool process() {
        return NRF_LOG_PROCESS();
    }

    bool hasKey() {
        return false;
//        return SEGGER_RTT_HasKey();
    }
    
    int getKey() {
        return 0;
//        return SEGGER_RTT_GetKey();
    }


    void selfTest() {
        NRF_LOG_INFO("Checking NRF LOG");
        NRF_LOG_INFO("Press any key to continue");
        while (!hasKey()) {
            process();
        }
        int key = getKey();
        NRF_LOG_INFO("Key successfully read: %d", key);
        process();
    }
}
}
