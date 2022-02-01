#include "die.h"

#include "app_timer.h"
#include "app_error.h"
#include "app_error_weak.h"

#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/a2d.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/i2c.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/gpiote.h"
#include "drivers_nrf/dfu.h"

#include "config/board_config.h"
#include "config/settings.h"

#include "drivers_hw/battery.h"
#include "drivers_hw/magnet.h"

#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "bluetooth/telemetry.h"

#include "animations/animation_cycle.h"
#include "data_set/data_set.h"

#include "modules/led_color_tester.h"
#include "modules/leds.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/animation_preview.h"
#include "modules/battery_controller.h"
#include "modules/behavior_controller.h"
#include "modules/hardware_test.h"
#include "modules/rssi_controller.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"

#include "nrf_drv_clock.h"

using namespace DriversNRF;
using namespace Config;
using namespace DriversHW;
using namespace Bluetooth;
using namespace Animations;
using namespace Modules;

#define APP_BLE_CONN_CFG_TAG    1

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

namespace Die
{
    void loopCycleAnimation() {
        static AnimationCycle anim;
        anim.type = Animation_Cycle;
        anim.duration = 25000;
        anim.faceMask = 0xFFFFF;
        anim.count = 1;
        anim.fade = 0;
        anim.rainbow = 0;

        NRF_LOG_INFO("Loop cycle animation");
        AnimController::play(&anim, nullptr, 0, true); // Loop forever!
    }

    void init() {
        //--------------------
        // Initialize NRF drivers
        //--------------------

        // Very first thing we want to init is the watchdog so we don't brick
        // later on if something bad happens.
        Watchdog::init();

        // Then the log system
        Log::init();

        // Then the scheduler, so we can avoid executing big stuff inside callbacks or interrupt handlers
        Scheduler::init();

        // Then the timers, used for periodic operations, like checking battery
        Timers::init();

        // Power manager handles going to sleep and resetting the board
        //PowerManager::init();

        // GPIO Interrupts, we use this to know when the accelerometer has a new reading available
        GPIOTE::init();

        // Analog to digital converter next, so we can identify the board we're dealing with
        A2D::init();
        
        // Initializte bluetooth
        Stack::init();

        // // Watchdog may setup a timer to verify app stability
        // Watchdog::initClearResetFlagTimer();

        // Add generic bluetooth data service
        MessageService::init();

        // Initialize the DFU service so we can upgrade the firmware without needing to reset the die
        DFU::init();

        // Now that the message service added its uuid to the softdevice, initialize the advertising
        Stack::initAdvertising();

        // Flash is needed to update settings/animations
        Flash::init();

        //--------------------
        // Fetch board configuration now, so we know how to initialize
        // the rest of the hardware (pins, led count, etc...)
        //--------------------
        // This will use the A2D converter to check the identifying resistor
        // on the board and determine what kind of die this is.
        BoardManager::init();

        // Magnet, so we know if ne need to go into quiet mode, this is no longer used
        //Magnet::init(); 
        
        // Now that we know which board we are, initialize the battery monitoring A2D
        A2D::initBoardPins();

        // Then we read user settings from flash, or set some defaults if none are found
        SettingsManager::init([] (bool result) {

            // The advertising name depends on settings
            Stack::initAdvertisingName();

            // Now that the settings are set, update custom advertising data such as die type and battery level
            Stack::initCustomAdvertisingData();

            // I2C is needed for the accelerometer, but depends on the board info to know which pins to use
            I2C::init();

            //--------------------
            // Initialize Hardware drivers
            //--------------------

            // Lights depend on board info as well
            LEDs::init();

            // Accel pins depend on the board info
            Accelerometer::init();

            // Battery sense pin depends on board info
            Battery::init();

            //--------------------
            // Initialize Modules
            //--------------------

            // Animation set needs flash and board info
            DataSet::init([] (bool result) {

            #if defined(DEBUG)
                // Useful for development
                LEDColorTester::init();
            #endif

                // Telemetry depends on accelerometer
                Telemetry::init();

                // Animation controller relies on animation set
                AnimController::init();

                // Battery controller relies on the battery driver
                BatteryController::init();

                bool loopAnim = (SettingsManager::getSettings()->debugFlags & (uint32_t)DebugFlags::LoopCycleAnimation) != 0;
                if (loopAnim) {
                    loopCycleAnimation();
                }
                else {
                    // Behavior Controller relies on all the modules
                    BehaviorController::init();
                }

                // Animation preview depends on bluetooth
                AnimationPreview::init();

                // Rssi controller requires the bluetooth stack
                RssiController::init();

            #if defined(DEBUG)
                HardwareTest::init();
            #endif
                // Start advertising!
                Stack::startAdvertising();

            #if defined(DEBUG_FIRMWARE)
                initDebugLogic();
                NRF_LOG_INFO("---------------");
            #else
                // Initialize main logic manager
                initMainLogic();
                NRF_LOG_INFO("---------------");

                // Entering the main loop! Play Hello! anim
                if (!loopAnim) {
                    BehaviorController::onDiceInitialized();
                }
            #endif
            });
        });
    }
}
