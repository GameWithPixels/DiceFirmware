#include "led_error_indicator.h"
#include "drivers_hw/neopixel.h"
#include "utils/Utils.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "drivers_nrf/watchdog.h"
#include "drivers_nrf/power_manager.h"
#include "modules/accelerometer.h"
#include "modules/anim_controller.h"

using namespace Config;
using namespace DriversHW;
using namespace DriversNRF;
using namespace Modules;

#define MAX_DELAY_BETWEEN_FEED_MS 500

namespace Modules::LEDErrorIndicator
{
    void WatchdogSafeDelayMs(uint32_t millis) {
        uint32_t count = millis / MAX_DELAY_BETWEEN_FEED_MS;
        uint32_t remainder = millis % MAX_DELAY_BETWEEN_FEED_MS;
        for (int i = 0; i < (int)count; ++i) {
            Watchdog::feed();
            nrf_delay_ms(MAX_DELAY_BETWEEN_FEED_MS);
        }
        Watchdog::feed();
        nrf_delay_ms(remainder);
    }

    void SetColor(uint32_t color) {
        // Color buffers
        uint32_t colors[MAX_LED_COUNT];
        for (int i = 0; i < MAX_LED_COUNT; ++i) {
            colors[i] = color;
        }
        NeoPixel::show(colors);
    }

    void BlinkColor(uint32_t color, uint32_t onTimeMillis, uint32_t offTimeMillis, int count) {
        // Color buffers
        uint32_t colors[MAX_LED_COUNT];
        uint32_t zeros[MAX_LED_COUNT];
        for (int i = 0; i < MAX_LED_COUNT; ++i) {
            colors[i] = color;
            zeros[i] = 0;
        }
        for (int i = 0; i < count; ++i) {
            NeoPixel::show(colors);
            WatchdogSafeDelayMs(onTimeMillis);
            NeoPixel::show(zeros);
            WatchdogSafeDelayMs(offTimeMillis);
        }
    }

    void BlinkSingle(uint32_t color, uint32_t onTimeMillis, uint32_t offTimeMillis, int count) {
        // Color buffers
        uint32_t colors[MAX_LED_COUNT];
        uint32_t zeros[MAX_LED_COUNT];
        for (int i = 0; i < MAX_LED_COUNT; ++i) {
            colors[i] = 0;
            zeros[i] = 0;
        }
        colors[BoardManager::getBoard()->debugLedIndex] = color;
        for (int i = 0; i < count; ++i) {
            NeoPixel::show(colors);
            WatchdogSafeDelayMs(onTimeMillis);
            NeoPixel::show(zeros);
            WatchdogSafeDelayMs(offTimeMillis);
        }
    }

    void ShowErrorAndHalt(ErrorType error) {

        // Before we do stuff we need to stop running modules that may be filling queues, such as the accelerometer
        Accelerometer::stop();
        AnimController::stop();

        // We don't really know the state of the led power pin, so set it
        nrf_gpio_pin_set(BoardManager::getBoard()->ledPowerPin);

        // Give enough time for the LEDs to power up
        WatchdogSafeDelayMs(10); // 5 should be enough but let's be safe

        WatchdogSafeDelayMs(500);

        // Array of colors
        switch (error)
        {
        case ErrorType_LEDs:
            BlinkColor(Utils::toColor(12, 0, 12), 3000, 100, 1);
            break;
        case ErrorType_BatterySense:
            BlinkColor(Utils::toColor(24, 0, 0), 3000, 100, 1);
            break;
        case ErrorType_BatteryCharge:
            BlinkSingle(Utils::toColor(1, 0, 0), 50, 50, 30);
            break;
        case ErrorType_Accelerometer:
            BlinkColor(Utils::toColor(0, 16, 8), 3000, 100, 1);
            break;
        case ErrorType_NTC:
            BlinkColor(Utils::toColor(16, 8, 0), 3000, 100, 1);
            break;
        default:
            break;
        }
        nrf_gpio_pin_clear(BoardManager::getBoard()->ledPowerPin);
        WatchdogSafeDelayMs(10);
        PowerManager::goToSystemOff();
    }
}