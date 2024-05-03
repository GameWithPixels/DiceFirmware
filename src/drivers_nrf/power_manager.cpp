#include "power_manager.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_power.h"
#include "nrf_bootloader_info.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "log.h"
#include "nrf_log.h"
#include "core/delegate_array.h"
#include "drivers_nrf/timers.h"

#define GPREGRET_ID 0
#define SLEEP_TIMEOUT_MS 30000 //  Comment to disable sleep timeout

#define POWER_EVENT_LOG(msg) NRF_LOG_INFO("Power event! Prepare %s", msg)

namespace DriversNRF::PowerManager
{
    PowerManagerClientMethod powerEventCallback = nullptr;

    APP_TIMER_DEF(sleepTimer);
    void triggerSleepMode(void* context);

    enum PowerManagerState {
        PowerManagerState_Normal = 0,
        PowerManagerState_Paused
    };

    PowerManagerState state = PowerManagerState_Normal;

    void init(PowerManagerClientMethod callback) {
        powerEventCallback = callback;

        ret_code_t err_code;
        err_code = nrf_pwr_mgmt_init();
        APP_ERROR_CHECK(err_code);

        #if defined(SLEEP_TIMEOUT_MS)
            Timers::createTimer(&sleepTimer, APP_TIMER_MODE_SINGLE_SHOT, triggerSleepMode);
            Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
        #endif

        state = PowerManagerState_Normal;

        NRF_LOG_DEBUG("Power Management init");
    }

    bool powerEventHandler(nrf_pwr_mgmt_evt_t event)
    {
        PowerManagerEvent pwrEvent = PowerManagerEvent_PrepareSleep;
        switch (event)
        {
            case NRF_PWR_MGMT_EVT_PREPARE_SYSOFF:
                POWER_EVENT_LOG("system off");
                pwrEvent = PowerManagerEvent_PrepareSysOff;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_WAKEUP:
                POWER_EVENT_LOG("wake up");
                pwrEvent = PowerManagerEvent_PrepareWakeUp;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_DFU:
                POWER_EVENT_LOG("wake DFU");
                pwrEvent = PowerManagerEvent_PrepareDFU;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_RESET:
                POWER_EVENT_LOG("wake reset");
                pwrEvent = PowerManagerEvent_PrepareReset;
                break;
        }

        // Notify clients
        powerEventCallback(pwrEvent);

        Log::process();
        return true;
    }

    /**@brief Register application shutdown handler with priority 0. */
    NRF_PWR_MGMT_HANDLER_REGISTER(powerEventHandler, 0);

    void triggerSleepMode(void* context) {
        NRF_LOG_INFO("PowerManager timeout => sleep");
        goToSleep();
    }

    void feed() {
        nrf_pwr_mgmt_feed();

        if (state == PowerManagerState_Normal) {
        // Restart the timer
        #if defined(SLEEP_TIMEOUT_MS)
            Timers::stopTimer(sleepTimer);
            Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
        #endif
        }
    }

    void update() {
        nrf_pwr_mgmt_run();
    }

    void goToSystemOff() {
        // Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        // Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_STAY_IN_SYSOFF);
    }

    void goToDeepSleep() {
        // Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        // Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }

    void goToSleep() {
        powerEventCallback(PowerManagerEvent_PrepareSleep);
    }

    void wakeFromSleep() {
        powerEventCallback(PowerManagerEvent_WakingUpFromSleep);

        // Restart the sleep timer
        #if defined(SLEEP_TIMEOUT_MS)
            Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
        #endif
    }

    void reset() {
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
    }

    bool checkFromSysOff() {
        // Get contents of reset register to check why we're booting
        // If we fail to read reason register, continue as if normal boot
        uint32_t reason = 0;
        ret_code_t ret = sd_power_reset_reason_get(&reason);
        bool fromSleep = false;
        if (ret == NRF_SUCCESS) {
            sd_power_reset_reason_clr(0xF000F);
            fromSleep = (reason & 0x00010000) != 0;
        }
        return fromSleep;
    }

    void pause() {

        if (state == PowerManagerState_Normal) {
        // Stop the timer
        #if defined(SLEEP_TIMEOUT_MS)
            Timers::stopTimer(sleepTimer);
        #endif
            state = PowerManagerState_Paused;
        }
    }

    void resume() {
        if (state == PowerManagerState_Paused) {
        // Restart the sleep timer
        #if defined(SLEEP_TIMEOUT_MS)
            Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
        #endif
            state = PowerManagerState_Normal;
        }
    }

}
