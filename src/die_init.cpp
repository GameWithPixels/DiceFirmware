#include "die.h"
#include "pixel.h"

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
#include "drivers_nrf/ppi.h"
#include "drivers_nrf/dfu.h"
#include "drivers_nrf/mcu_temperature.h"

#include "config/board_config.h"
#include "config/settings.h"
#include "config/value_store.h"

#include "drivers_hw/battery.h"
#include "drivers_hw/coil.h"
#include "drivers_hw/ntc.h"

#include "bluetooth/bluetooth_stack.h"
#include "bluetooth/bluetooth_custom_advertising_data.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bulk_data_transfer.h"
#include "bluetooth/telemetry.h"

#include "animations/animation_cycle.h"
#include "data_set/data_set.h"

#include "handlers/battery_notifications.h"
#include "handlers/roll_notifications.h"
#include "handlers/rssi_notifications.h"
#include "handlers/power_event.h"
#include "handlers/set_led_color.h"
#include "handlers/store_value.h"
#include "handlers/who_are_you.h"

#include "modules/leds.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"
#include "modules/instant_anim_controller.h"
#include "modules/battery_controller.h"
#include "modules/behavior_controller.h"
#include "modules/charger_proximity.h"
#include "modules/temperature.h"
#include "modules/validation_manager.h"
#include "modules/led_error_indicator.h"
#include "modules/attract_mode_controller.h"
#include "modules/user_mode_controller.h"
#include "modules/discharge_controller.h"

#include "utils/Utils.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#include "nrf_power.h"
#include "nrf_drv_clock.h"

using namespace DriversNRF;
using namespace Config;
using namespace DriversHW;
using namespace Bluetooth;
using namespace Animations;
using namespace Modules;

#define APP_BLE_CONN_CFG_TAG    1

namespace Die
{
    // Callback for calling PowerManager::feed to prevent sleep mode due to animations
    void feed(void* param, Accelerometer::RollState prevState, int prevFace, Accelerometer::RollState newState, int newFace)  {
        PowerManager::feed();
    }

    void init() {
        //--------------------
        // Initialize NRF drivers
        // We don't expect NRF drivers to error unless because of a firmware bug
        // because all NRF drivers are internal to the NRF chip.
        //--------------------

        // ** Reminder ** Update above list of validation checks if adding new functions

        // Very first thing we want to init is the watchdog so we don't brick
        // later on if something bad happens.
        Watchdog::init();

        // Then the log system
        Log::init();

        // Display reset reason bits
        #if defined(NRF_LOG_ENABLED)
        uint32_t resetReas = nrf_power_resetreas_get();
        nrf_power_resetreas_clear(0xFFFFFFFF);
        if (resetReas != 0) {
            if ((resetReas & (1 << 0)) != 0) { NRF_LOG_WARNING("Reset Reason - %s", "PIN RESET"); }
            if ((resetReas & (1 << 1)) != 0) { NRF_LOG_ERROR("Reset Reason - %s", "WATCHDOG"); }
            if ((resetReas & (1 << 2)) != 0) { NRF_LOG_INFO("Reset Reason - %s", "SYSTEM REQUEST"); }
            if ((resetReas & (1 << 3)) != 0) { NRF_LOG_ERROR("Reset Reason - %s", "LOCKUP"); }
            if ((resetReas & (1 << 16)) != 0) { NRF_LOG_INFO("Reset Reason - %s", "WAKE FROM SYSOFF"); }
            if ((resetReas & (1 << 18)) != 0) { NRF_LOG_INFO("Reset Reason - %s", "DEBUG"); }
        }
        #endif

        // Then the scheduler, so we can avoid executing big stuff inside callbacks or interrupt handlers
        Scheduler::init();

        // Then the timers, used for periodic operations, like checking battery
        Timers::init();

        // Power manager handles going to sleep and resetting the board
        PowerManager::init(onPowerEvent);

        // GPIO Interrupts, we use this to know when the accelerometer has a new reading available
        GPIOTE::init();

        // Analog to digital converter next, so we can identify the board we're dealing with
        A2D::init();

        // Peripheral to peripheral Interconnect, used by the battery charge detection
        PPI::init();
        
        // Initialize bluetooth
        Stack::init();

        //// Watchdog may setup a timer to verify app stability
        //Watchdog::initClearResetFlagTimer();

        // Add generic bluetooth data service
        MessageService::init();

        // Initialize the DFU service so we can upgrade the firmware without needing to reset the die
        DFU::init();

        // Flash is needed to update settings/animations
        Flash::init();

        //--------------------
        // Fetch board configuration now, so we know how to initialize
        // the rest of the hardware (pins, led count, etc...)
        //--------------------
        // This will use the A2D converter to check the identifying resistor
        // on the board and determine what kind of die this is.
        BoardManager::init();

        // Then we read user settings from flash, or set some defaults if none are found
        SettingsManager::init([] () {

            // I2C is needed for the accelerometer, but depends on the board info to know which pins to use
            I2C::init();

            //--------------------
            // Initialize Hardware drivers
            // Hardware drivers can fail because of physical problems (bad pcb, bad components, damage, etc...)
            //--------------------

            // Battery sense pin depends on board info
            // on fail blink red one long time then power off
            static bool batteryInitRet = false;
            batteryInitRet = Battery::init();

            static bool coilInitRet = false;
            coilInitRet = Coil::init();

            // Accel pins depend on the board info
            // on fail blink 2 short times then power off
            static bool accInitRet = false;
            Accelerometer::init([](bool accInitRetParam) {
                accInitRet = accInitRetParam;

                // Battery Temperature Module
                // on fail blink red 3 short times then power off
                NTC::init();

                // Temperature sensor
                MCUTemperature::init();

                // Temperature Module
                Temperature::init([] (bool tempInitRetParam) {

                    static bool tempInitRet = false;
                    tempInitRet = tempInitRetParam;
                    // Battery controller relies on the battery driver
                    BatteryController::init();

                    // Charger proximity translates info from the battery controller
                    ChargerProximity::init();

                    // Lights depend on board info as well
                    LEDs::init([](bool ledInitRet) {
                        const bool engineeringSample = !ValidationManager::inValidation() && !ValueStore::hasValidationTimestamp();
                        if (!engineeringSample) {
                            // If LED init failed, we will "try" to turn LEDs on, hoping the problem is simply an led chain thing
                            if (!ledInitRet) {
                                LEDErrorIndicator::ShowErrorAndHalt(LEDErrorIndicator::ErrorType_LEDs);
                            }

                            if (!tempInitRet) {
                                LEDErrorIndicator::ShowErrorAndHalt(LEDErrorIndicator::ErrorType_NTC);
                            }
                        }

                        // Now that we have LEDs, indicate battery or acc errors
                        if (!batteryInitRet || !coilInitRet) {
                            LEDErrorIndicator::ShowErrorAndHalt(LEDErrorIndicator::ErrorType_BatterySense);
                        }

                        if (!accInitRet) {
                            LEDErrorIndicator::ShowErrorAndHalt(LEDErrorIndicator::ErrorType_Accelerometer);
                        }

                        // Animation set needs flash and board info
                        DataSet::init([] () {

                            // Telemetry depends on accelerometer
                            Telemetry::init();

                            // Animation controller relies on animation set
                            AnimController::init();

                            //--------------------
                            // Initialize Bluetooth Advertising Data + Name
                            //--------------------

                            // Now that the message service added its uuid to the SoftDevice, initialize the advertising
                            Stack::initAdvertising();

                            // Initialize custom advertising data handler
                            CustomAdvertisingDataHandler::init();

                            auto runMode = Pixel::getCurrentRunMode();
                            if (runMode == Pixel::RunMode_User || runMode == Pixel::RunMode_Invalid) {
                                // Want to prevent sleep mode due to animations while not in validation
                                Accelerometer::hookRollState(feed, nullptr);
                            }

                            // Behavior Controller relies on all the modules
                            BehaviorController::init(false, true, true);

                            // Instant Animation Controller preview depends on bluetooth
                            InstantAnimationController::init();

                            // Another module used in testing
                            DischargeController::init();

                            // Allow the die to go "silent" and not play animations based on behavior rules, but only when told to
                            UserModeController::init();

                            // Initialize various message handlers
                            Handlers::PowerEvent::init();
                            Handlers::SetLEDColor::init();
                            Handlers::StoreValue::init();
                            Handlers::WhoAreYou::init();
                            Handlers::BatteryNotifications::init();
                            Handlers::RollNotifications::init();
                            Handlers::RssiNotifications::init();

                            // Initialize common message handlers
                            Die::initMainLogic();

                            // Always init validation manager so it handles the ExitValidation message
                            ValidationManager::init();

                            // Start advertising!
                            Stack::startAdvertising();

                            // Entering the main loop! Play Hello! anim if in validation mode
                            switch (runMode) {
                                case Pixel::RunMode_Validation:
                                    ValidationManager::onPixelInitialized();
                                    break;
                                case Pixel::RunMode_Attract:
                                    AttractModeController::init();
                                    break;
                                default:
                                    Die::initDieLogic();
                                    BehaviorController::onPixelInitialized();
                                    Timers::setDelayedCallback([](void* ignore) {
                                        BehaviorController::EnableAccelerometerRules();
                                    }, nullptr, 1000);
                                    break;
                            }

                            NRF_LOG_INFO("----- Device initialized! -----");
                        });
                    });
                });
            });
        });
    }
}
