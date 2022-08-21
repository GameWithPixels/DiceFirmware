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

#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "bluetooth/telemetry.h"

#include "animations/animation_cycle.h"
#include "animations/animation_noise.h"
#include "data_set/data_set.h"

#include "modules/led_color_tester.h"
#include "modules/leds.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/animation_preview.h"
#include "modules/instant_anim_controller.h"
#include "modules/battery_controller.h"
#include "modules/behavior_controller.h"
#include "modules/hardware_test.h"
#include "modules/rssi_controller.h"
#include "utils/Utils.h"
#include "modules/validation_manager.h"

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
    // Callback for calling PowerManager::feed to prevent sleep mode due to animations
    void feed(void* token)  {
        PowerManager::feed();
    }

    /// ***********************************************************************
    /// Init function validation checks:
    ///
    ///     - Watchdog::init():
    ///         * if watchdog driver init fails
    ///         * if channel allocation fails
    ///
    ///     - Log::init():
    ///         * if log driver initialization fails
    ///
    ///     - Scheduler::init():
    ///         * if scheduler driver init fails
    ///
    ///     - Timers::init():
    ///         * if timer driver init fails
    ///
    ///     - GPIOTE::init():
    ///         * if gpiote driver init fails
    ///
    ///     - A2D::init():
    ///         * if saadc driver init fails
    ///
    ///     - Stack::init():
    ///         * if softdevice enable fails
    ///         * if BLE stack config fails
    ///         * if BLE enable fails
    ///         * if setting GAP connection parameters fails
    ///         * if GATT init fails
    ///
    ///     - MessageSevice::init():
    ///         * if adding custom base UUID fails
    ///         * if adding GATT service fails   
    ///         * if adding RX characteristic fails
    ///         * if adding TX characteristic fails
    ///
    ///     - DFU::init():
    ///         * if aysnc SVCI init fails
    ///         * if DFU driver init failse
    ///
    ///     - Stack::initAdvertising():
    ///         * if advertising init fails
    ///         * if queued writes module init fails
    ///         * if connection parameters module init fails       
    ///
    ///     - Flash::init():
    ///         * if fstorage init fails
    ///
    ///     - BoardManager::init():
    ///         * no validation checks
    ///         * may want to fail-safe for unexpectedly 
    ///             high voltage measurement?
    ///
    ///     - A2D::initBoardPins():
    ///         * no validation checks (needed?)
    ///
    ///     - SettingsManager::init():
    ///         * if Flash::write() fails to write dataset
    ///     
    ///     - Stack::initAdvertisingName():
    ///         * if setting GAP device name fails
    ///
    ///     - Stack::initCustomAdvertisingData():
    ///         * if advertising data update fails
    ///         * if advertising data encode fail (SDK_VER 12)
    ///
    ///     - I2C::init():
    ///         * if twi driver init fails
    ///
    ///     - LEDs::init():
    ///         * if pwm_init fails (neopixel)
    ///         * maybe check getSettings() return value?
    ///
    ///     - Accelerometer::init():
    ///         * maybe throw error for invalid accel model 
    ///             in init() and start()?
    ///
    ///     - Battery::init():
    ///         * maybe use self-test?
    ///
    ///     - DataSet::init:
    ///         * nothing is checked?
    ///
    ///     - Telemetry::init():
    ///         * nothing is checked?
    ///
    ///     - AnimController::init():
    ///         * if create animControllerTimer fails 
    ///         * if start animControllerTimer fails
    ///
    ///     - BatteryController::init():
    ///         * if create batteryControllerTimer fails
    ///         * if start batteryControllerTimer fails
    ///
    ///     - BehaviorController::init():
    ///         * nothing is checked?
    ///
    ///     - AnimationPreview::init():
    ///         * nothing is checked?
    ///
    ///     - RssiController::init():
    ///         * nothing is checked?
    ///
    ///     - HardwareTest::init():
    ///         * this test does not seem necessary for validation
    ///
    ///     - Stack::startAdvertising():
    ///         * if advertising data update fails
    ///         * if advertising data encode fails (SDK_VER 12)
    ///         * if advertising start fails
    ///
    ///     - initMainLogic():
    ///         * nothing is checked?
    ///
    ///     - BehaviorController::onDiceInitialized():
    ///         * will probably use alternative function 
    ///             for validation blinks
    ///
    /// ***********************************************************************

    void init() {
        //--------------------
        // Initialize NRF drivers
        //--------------------

        // ** Reminder ** Update above list of validation checks if adding new functions

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
        PowerManager::init();

        // GPIO Interrupts, we use this to know when the accelerometer has a new reading available
        GPIOTE::init();

        // Analog to digital converter next, so we can identify the board we're dealing with
        A2D::init();
        
        // Initializte bluetooth
        Stack::init();

        //// Watchdog may setup a timer to verify app stability
        //Watchdog::initClearResetFlagTimer();

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

                const bool inValidation = ValidationManager::inValidation();
                if (!inValidation)
                {
                    // Want to prevent sleep mode due to animations while not in validation
                    AnimController::hook(feed, nullptr);
                }

                // Behavior Controller relies on all the modules
                BehaviorController::init();

                // Animation Preview depends on bluetooth
                AnimationPreview::init();

                // Instant Animation Controller preview depends on bluetooth
                InstantAnimationController::init();

                // Rssi controller requires the bluetooth stack
                RssiController::init();

            // #if defined(DEBUG)
            //     HardwareTest::init();
            // #endif
                // Start advertising!
                Stack::startAdvertising();

                // Initialize common logic
                initMainLogic();

                // Skip registering unecessary BLE messages in validation mode
                if (!inValidation)
                {
                    // Initialize main die logic
                    initDieLogic();
                }

                // Entering the main loop! Play Hello! anim
                // if in validation mode
                if (inValidation) {
                    ValidationManager::init();
                    ValidationManager::onDiceInitialized();
                } else {
                    BehaviorController::onDiceInitialized();
                }

                static AnimationNoise noiseAnim;

                noiseAnim.type = Animation_Noise;
                noiseAnim.duration = 40000;
                noiseAnim.faceMask = ANIM_FACEMASK_ALL_LEDS;
                noiseAnim.gradientTrackOffset = 0;
                noiseAnim.flashDelay = 80;
                noiseAnim.flashSpeed = 15;
                AnimController::play(&noiseAnim, nullptr, 0, false);
                


                NRF_LOG_INFO("----- Device initialized! -----");
            });
        });
    }
}
