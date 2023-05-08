#include "battery.h"
#include "nrf_assert.h"
#include "nrf_log.h"
#include "board_config.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_saadc.h"

#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"

#include "drivers_nrf/gpiote.h"
#include "drivers_nrf/a2d.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/scheduler.h"
#include "core/delegate_array.h"
#include "modules/validation_manager.h"

using namespace DriversNRF;
using namespace Config;

#define MAX_BATTERY_CLIENTS 2
#define BATTERY_CHARGE_PIN_TIMER 1000 // milliseconds
#define VBAT_LOW_THRESHOLD 3000 // x0.001 Volts

namespace DriversHW
{
namespace Battery
{
    const int32_t vBatMultTimes1000 = 1400; // Voltage divider 10M over 4M
    const int32_t vLEDMultTimes1000 = 1400; // Voltage divider 10M over 4M
    const int32_t vCoilMultTimes1000 = 2000; // Voltage divider 4M over 4M

	DelegateArray<ClientMethod, MAX_BATTERY_CLIENTS> clients;

    static const nrf_drv_timer_t battTimer = NRF_DRV_TIMER_INSTANCE(1);
    static nrf_ppi_channel_t m_ppi_channel1;
    static nrf_drv_gpiote_in_config_t in_config;
    static uint8_t statePin = 0xFF; // Cached from board manager in init
    bool charging = false;
    bool forceDisableChargingState = false;

    void battTimerHandler(nrf_timer_event_t event_type, void* p_context);
    void pinHiToLoHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
    void pinLoToHiHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
    void handleChargeEvent(void * p_event_data, uint16_t event_size);

    bool checkChargingInternal() {
        return nrf_gpio_pin_read(statePin) == 0;
    }

    void battTimerHandler(nrf_timer_event_t event_type, void* p_context) {

        // We're going to change the event associated with the charge pin, so disable the event first
        nrf_drv_gpiote_in_event_disable(statePin);

        // Only way to change the handler called is to uninit and then reinit the pin unfortunately
        // Maybe in the future we'll go fiddle with the registers directly to make it a bit more efficient
        nrf_drv_gpiote_in_uninit(statePin);

        // We don't need the timer anymore
        nrf_drv_timer_disable(&battTimer);

        // Or the PPI channel to auto-reset it if the state pin keeps toggling
        ret_code_t err_code = nrf_drv_ppi_channel_disable(m_ppi_channel1);
        APP_ERROR_CHECK(err_code);

        // Read the charging pin, we know it hasn't toggled in a while, but we don't know if it is
        // high or low. What we do depends on that! Note: this updates our internal charging state variable as well.
        charging = checkChargingInternal();
        if (charging) {
            // Setup sense polarity for when the signal goes back high and attach our Lo to Hi handler
            in_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
            err_code = nrf_drv_gpiote_in_init(statePin, &in_config, pinLoToHiHandler);
            APP_ERROR_CHECK(err_code);
        } else {
            // Setup sense polarity for when the signal goes low and set an event handler
            in_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
            err_code = nrf_drv_gpiote_in_init(statePin, &in_config, pinHiToLoHandler);
            APP_ERROR_CHECK(err_code);

            // Notify clients
            ChargingEvent evt = ChargingEvent_ChargeStop;
            Scheduler::push(&evt, sizeof(ChargingEvent), handleChargeEvent);
        }

        // Enable pin event, making sure to allow the handler to be called
        nrf_drv_gpiote_in_event_enable(statePin, true);
    }

    void pinHiToLoHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {

        // We're going to change the event associated with the charge pin, so disable the event first
        nrf_drv_gpiote_in_event_disable(statePin);

        // Only way to change the handler called is to uninit and then reinit the pin unfortunately
        // Maybe in the future we'll go fiddle with the registers directly to make it a bit more efficient
        nrf_drv_gpiote_in_uninit(statePin);

        // Update internal state
        charging = true;

        // Setup sense polarity for when the signal goes back high and attach our Lo to Hi handler
        in_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
        ret_code_t err_code = nrf_drv_gpiote_in_init(statePin, &in_config, pinLoToHiHandler);
        APP_ERROR_CHECK(err_code);

        // Enable pin event, making sure to allow the handler to be called
        nrf_drv_gpiote_in_event_enable(statePin, true);

        // Notify clients
        ChargingEvent evt = ChargingEvent_ChargeStart;
        Scheduler::push(&evt, sizeof(ChargingEvent), handleChargeEvent);
    }

    void pinLoToHiHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {

        // We're going to change the event associated with the charge pin, so disable the event first
        nrf_drv_gpiote_in_event_disable(statePin);

        // Only way to change the handler called is to uninit and then reinit the pin unfortunately
        // Maybe in the future we'll go fiddle with the registers directly to make it a bit more efficient
        nrf_drv_gpiote_in_uninit(statePin);

        // Setup sense polarity for when the signal goes back high,
        // but instead of triggering a code handler, use it to reset timer2
        // Only when timer2 expires will we fire off an event handler.
        // This is our way of filtering the input to ignore frequent toggles.
        in_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
        ret_code_t err_code = nrf_drv_gpiote_in_init(statePin, &in_config, nullptr);
        APP_ERROR_CHECK(err_code);

        // Enable PPI channel to the timer
        err_code = nrf_drv_ppi_channel_enable(m_ppi_channel1);
        APP_ERROR_CHECK(err_code);

        // Enable pin event
        nrf_drv_gpiote_in_event_enable(statePin, false);

        // Start the timer
        nrf_drv_timer_enable(&battTimer);
    }


    bool init() {
        // Set charger and fault pins as input

        // Drive the status pin down for a moment
        statePin = BoardManager::getBoard()->chargingStatePin;

        // Read battery level and convert
        nrf_gpio_cfg_input(statePin, NRF_GPIO_PIN_PULLUP);
        // pull up inputs take a second for the voltage to rise up, so wait before reading it
        nrf_delay_us(1);
        charging = checkChargingInternal();

        // By default we don't want to touch anything wrt charge programming
        setDisableChargingOverride(false);

        // We'll re-use this config struct over and over, set up the starting parameters now
        in_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
        in_config.pull = NRF_GPIO_PIN_PULLUP;
        in_config.is_watcher = false;
        in_config.hi_accuracy = true;
        in_config.skip_gpio_setup = true;   // Don't reset gpio state, otherwise it causes the pull-up to be
                                            // disabled briefly and not necessarily be in a valid state when
                                            // we try to read it.

        ret_code_t err_code = NRF_SUCCESS;
        if (charging) {
            // Setup sense polarity for when the signal goes back high and attach our Lo to Hi handler
            in_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
            err_code = nrf_drv_gpiote_in_init(statePin, &in_config, pinLoToHiHandler);
            APP_ERROR_CHECK(err_code);
        } else {
            // Setup sense polarity for when the signal goes low and set an event handler
            in_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
            err_code = nrf_drv_gpiote_in_init(statePin, &in_config, pinHiToLoHandler);
            APP_ERROR_CHECK(err_code);
        }

        // Configure PPI channel to reset timer on pin hi to lo transition

        // Find an unused channel
        err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel1);
        APP_ERROR_CHECK(err_code);

        // Configure it to connect gpio to timer
        auto charge_state_pin_event = nrf_drv_gpiote_in_event_addr_get(statePin);
        auto timer_clear_counter_task = nrf_drv_timer_task_address_get(&battTimer, NRF_TIMER_TASK_CLEAR);
        err_code = nrf_drv_ppi_channel_assign(m_ppi_channel1, charge_state_pin_event, timer_clear_counter_task);
        APP_ERROR_CHECK(err_code);

        // Setup our timer now, we'll reset and start / stop as needed
        nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
        timer_cfg.frequency = NRF_TIMER_FREQ_31250Hz; // Lowest available frequency because our filtering time is large
        timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_16; // We need enough bits to count to 1 second, at 31250Hz, that is 31250 :)
        err_code = nrf_drv_timer_init(&battTimer, &timer_cfg, battTimerHandler);
        APP_ERROR_CHECK(err_code);

        // Setup the timer to use the hardware timer channel 1 (0 is used byt the softdevice),
        // make the compare time 1 second and stop the timer when reached.
        nrf_drv_timer_extended_compare(&battTimer, NRF_TIMER_CC_CHANNEL1, nrf_drv_timer_ms_to_ticks(&battTimer, BATTERY_CHARGE_PIN_TIMER), NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);

        // Enable gpio event so we get an interrupt when the state pin toggles
        nrf_drv_gpiote_in_event_enable(statePin, true);

        int32_t vCoilTimes1000 = checkVCoilTimes1000();
        int32_t vBatTimes1000 = checkVBatTimes1000();

        bool success = vBatTimes1000 > VBAT_LOW_THRESHOLD;
        if (!success) {
            NRF_LOG_ERROR("Battery Voltage too low: %d.%03d", vBatTimes1000 / 1000, vBatTimes1000 % 1000);
        }

        NRF_LOG_INFO("Battery init");
        NRF_LOG_INFO("  Voltage: %d.%03d", vBatTimes1000 / 1000, vBatTimes1000 % 1000);
        NRF_LOG_INFO("  Charging: %d", (charging ? 1 : 0));
        NRF_LOG_INFO("  VCoil: %d.%03d", vCoilTimes1000 / 1000, vCoilTimes1000 % 1000);

        #if DICE_SELFTEST && BATTERY_SELFTEST
        selfTest();
        #endif

        return success;
    }

    int32_t checkVBatTimes1000() {
        int32_t ret = A2D::readVBatTimes1000() * vBatMultTimes1000 / 1000;
        return ret;
    }

    int32_t checkVCoilTimes1000() {
        int32_t ret = A2D::read5VTimes1000() * vCoilMultTimes1000 / 1000;
        return ret;
    }

    bool checkCharging() {
        return charging;
    }

    void handleChargeEvent(void * p_event_data, uint16_t event_size) {
        ASSERT(event_size == sizeof(ChargingEvent));
        ChargingEvent* evt = (ChargingEvent*)p_event_data;

        switch (*evt)
        {
            case ChargingEvent_ChargeStart:
                NRF_LOG_DEBUG("Battery started charging");
                break;
            case ChargingEvent_ChargeStop:
            default:
                NRF_LOG_DEBUG("Battery stopped charging");
                break;
        }

		// Notify clients
		for (int i = 0; i < clients.Count(); ++i)
		{
			clients[i].handler(clients[i].token, *evt);
		}
    }

    void setDisableChargingOverride(bool disable) {
        auto progPin = BoardManager::getBoard()->progPin;
        if (progPin != 0xFF) {
            forceDisableChargingState = disable;
            if (disable) {
                nrf_gpio_cfg_output(progPin);
                nrf_gpio_pin_set(progPin);
            } else {
                nrf_gpio_cfg_default(progPin);
            }
        }
    }

    bool getDisableChargingOverride() {
        return forceDisableChargingState;
    }


	/// <summary>
	/// Method used by clients to request callbacks when battery changes state
	/// </summary>
	void hook(ClientMethod callback, void* parameter) {
		if (!clients.Register(parameter, callback))
		{
			NRF_LOG_ERROR("Too many battery hooks registered.");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting battery callbacks
	/// </summary>
	void unHook(ClientMethod callback) {
		clients.UnregisterWithHandler(callback);
	}

	/// <summary>
	/// Method used by clients to stop getting battery callbacks
	/// </summary>
	void unHookWithParam(void* param) {
		clients.UnregisterWithToken(param);
	}
}
}