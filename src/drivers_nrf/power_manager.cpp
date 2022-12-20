#include "power_manager.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_power.h"
#include "nrf_bootloader_info.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "log.h"
#include "core/delegate_array.h"
#include "drivers_nrf/timers.h"

#define MAX_POWER_CLIENTS 4
#define GPREGRET_ID 0
#define SLEEP_TIMEOUT_MS 30000
#define WATCHDOG_TRIGGERED_RESET    0x0001 // This bit gets set the first time the watchdog resets the mcu
#define CLEAR_SETTINGS_AND_DATASET  0x0002 // If above bit was still set when watchdog triggered reset,
                                           // then this bit gets set to tell boot up sequence to reprogram flash.

namespace DriversNRF::PowerManager
{
	DelegateArray<PowerManagerClientMethod, MAX_POWER_CLIENTS> clients;

	APP_TIMER_DEF(sleepTimer);
    void triggerSleepMode(void* context);

    void init() {
        ret_code_t err_code;
        err_code = nrf_pwr_mgmt_init();
        APP_ERROR_CHECK(err_code);

		Timers::createTimer(&sleepTimer, APP_TIMER_MODE_SINGLE_SHOT, triggerSleepMode);
		Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);

        NRF_LOG_DEBUG("Power Management init");
    }

    void hook(PowerManagerClientMethod method, void* param) {
		clients.Register(param, method);
    }

    void unHook(PowerManagerClientMethod client) {
        clients.UnregisterWithHandler(client);
    }

    bool powerEventHandler(nrf_pwr_mgmt_evt_t event)
    {
        PowerManagerEvent pwrEvent = PowerManagerEvent_PrepareSleep;
        switch (event)
        {
            case NRF_PWR_MGMT_EVT_PREPARE_SYSOFF:
                NRF_LOG_INFO("Power event! Prepare system off");
                pwrEvent = PowerManagerEvent_PrepareSysOff;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_WAKEUP:
                NRF_LOG_INFO("Power event! Prepare wake up");
                pwrEvent = PowerManagerEvent_PrepareWakeUp;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_DFU:
                NRF_LOG_INFO("Power event! Prepare wake DFU");
                pwrEvent = PowerManagerEvent_PrepareDFU;
                break;

            case NRF_PWR_MGMT_EVT_PREPARE_RESET:
                NRF_LOG_INFO("Power event! Prepare wake reset");
                pwrEvent = PowerManagerEvent_PrepareReset;
                break;
        }

        // Notify clients
        for (int i = 0; i < clients.Count(); ++i) {
            clients[i].handler(clients[i].token, pwrEvent);
        }

        Log::process();
        return true;
    }

    /**@brief Register application shutdown handler with priority 0. */
    NRF_PWR_MGMT_HANDLER_REGISTER(powerEventHandler, 0);

    void triggerSleepMode(void* context) {
        NRF_LOG_INFO("PowerManager timeout! Going to Sleep");
        goToSleep();
    }

    void feed() {
        nrf_pwr_mgmt_feed();

        // Restart the timer
        Timers::stopTimer(sleepTimer);
        Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
    }

    void update() {
        nrf_pwr_mgmt_run();
    }

    void goToSystemOff() {
        // Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        // Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }

    void goToSleep() {
        // Notify clients
        for (int i = 0; i < clients.Count(); ++i) {
            clients[i].handler(clients[i].token, PowerManagerEvent_PrepareSleep);
        }
    }

    void wakeFromSleep() {
        // Notify clients
        for (int i = 0; i < clients.Count(); ++i) {
            clients[i].handler(clients[i].token, PowerManagerEvent_WakingUpFromSleep);
        }
        Timers::startTimer(sleepTimer, APP_TIMER_TICKS(SLEEP_TIMEOUT_MS), NULL);
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

}
