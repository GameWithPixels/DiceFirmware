#include "leds.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "core/delegate_array.h"
#include "drivers_hw/neopixel.h"
#include "drivers_hw/battery.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_nrf/gpiote.h"
#include "battery_controller.h"
#include "utils/Utils.h"
#include "string.h" // for memset

using namespace Config;
using namespace DriversHW;
using namespace DriversNRF;


#define OFFSET_RED 2
#define OFFSET_GREEN 1
#define OFFSET_BLUE 0

#define MAX_APA102_CLIENTS 2
#define LOW_BATT_LED_INTENSITY_DIVISOR 32

namespace Modules::LEDs
{
    static DelegateArray<LEDClientMethod, MAX_APA102_CLIENTS> ledPowerClients;
    static uint8_t powerPin;
    static uint8_t numLed = 0;
    static bool powerOn = false;
    static uint32_t pixels[MAX_COUNT];

    void show();

    void setPowerOn(Timers::DelayedCallback callback, void* parameter);
    void setPowerOff();

    typedef void (*TestLEDCallback)(bool success);
    void testLEDReturn(TestLEDCallback callback);

    void init(InitCallback callback) {
        static InitCallback _callback; // Don't initialize this static inline because it would only do it on first call!
        _callback = callback;
        auto board = BoardManager::getBoard();
        NeoPixel::init();

        // Initialize Power pin
        powerPin = board->ledPowerPin;

        // Nothing to validate
        nrf_gpio_cfg_output(powerPin);
        nrf_gpio_pin_clear(powerPin);
        powerOn = false;

        // Initialize out LED return pin
        nrf_gpio_cfg_default(board->ledReturnPin);

        // Initialize our color array
        memset(pixels, 0, MAX_COUNT * sizeof(uint32_t));
        numLed = board->ledCount;

        if (BatteryController::getState() != BatteryController::State_Empty &&
            BatteryController::getState() != BatteryController::State_Low &&
            BatteryController::getState() != BatteryController::State_ChargingLow) {
            testLEDReturn([](bool success) {
                if (success) {
                    NRF_LOG_DEBUG("LEDs init, powerPin=%d", (int)powerPin);
                } else {
                    NRF_LOG_ERROR("LED Return not detected");
                }
                _callback(success);
            });
        } else {
            // Don't try to turn LED power on, voltage is too low
            _callback(true);
        }
    }

    
    void testLEDReturn(TestLEDCallback callback) {
        static TestLEDCallback _callback; // Don't initialize this static inline because it would only do it on first call!
        _callback = callback;
        
        // test LED return
        setPowerOn([](void* ignore) {

            // Now that supposedly LEDs are powered on, set interrupt pin
            // to detect the output of the last LED toggling
            static bool ledReturnDetected;

            // Initialize on separate line to make sure it happens every call, as this is a static variable
            ledReturnDetected = false;
            GPIOTE::enableInterrupt(
                BoardManager::getBoard()->ledReturnPin,
                NRF_GPIO_PIN_PULLUP,
                NRF_GPIOTE_POLARITY_TOGGLE,
                [](uint32_t pin, nrf_gpiote_polarity_t action) {
                    ledReturnDetected = true;
                });

            // Set a timer to check for LED return
            Timers::setDelayedCallback([] (void* ignore) {

                // When the callback happens:

                // Turn off the interrupt handler
                GPIOTE::disableInterrupt(BoardManager::getBoard()->ledReturnPin);
                nrf_gpio_cfg_default(BoardManager::getBoard()->ledReturnPin);

                // Turn off LED power
                setPowerOff();

                // Trigger callback with the result
                _callback(ledReturnDetected);
            }, nullptr, 5);

            // Have LEDs test the return
            NeoPixel::testLEDReturn();
        }, nullptr);
    }

    void clear() {
        memset(pixels, 0, MAX_COUNT * sizeof(uint32_t));
        show();
    }

    void setPixelColor(uint16_t n, uint32_t c) {
        if (n < numLed) {
            pixels[n] = c;
            show();
        }
    }

    void setAll(uint32_t c) {
        for (int i = 0; i < numLed; ++i) {
            pixels[i] = c;
        }
        show();
    }

    void setPixelColors(int* indices, uint32_t* colors, int count) {
        for (int i = 0; i < count; ++i) {
            int n = indices[i];
            if (n < numLed) {
                pixels[n] = colors[i];
            }
        }
        show();
    }

    void setPixelColors(uint32_t* colors) {
        memcpy(pixels, colors, numLed * sizeof(uint32_t));
        show();
    }


    void hookPowerState(LEDClientMethod method, void* param) {
        ledPowerClients.Register(param, method);
    }

    void unHookPowerState(LEDClientMethod method) {
        ledPowerClients.UnregisterWithHandler(method);
    }

    void unHookPowerStateWithParam(void* param) {
        ledPowerClients.UnregisterWithToken(param);
    }

    bool isPixelDataZero() {
        for (int i = 0; i < numLed; ++i) {
            if (pixels[i] != 0) {
                return false;
            }
        }
        return true;
    }

    void clampColors() {
        for (int i = 0; i < numLed; ++i) {
            uint32_t color = pixels[i];
            // Clamp intensity
            uint8_t r = Utils::getRed(color);
            uint8_t g = Utils::getGreen(color);
            uint8_t b = Utils::getBlue(color);
            r = (r + LOW_BATT_LED_INTENSITY_DIVISOR - 1) / LOW_BATT_LED_INTENSITY_DIVISOR;
            g = (g + LOW_BATT_LED_INTENSITY_DIVISOR - 1) / LOW_BATT_LED_INTENSITY_DIVISOR;
            b = (b + LOW_BATT_LED_INTENSITY_DIVISOR - 1) / LOW_BATT_LED_INTENSITY_DIVISOR;
            pixels[i] = Utils::toColor(r,g,b);
        }
    }

    void show() {
        // Do we want all the LEDs to be off?
        if (isPixelDataZero()) {
            setPowerOff();
        } else {
            // Only turn power on if Battery is strong enough
            if (BatteryController::getState() != BatteryController::State_Empty) {
                // Turn power on so we display something!!!
                setPowerOn([](void* ignore) {
                    // Check battery and coil voltage to determine if we should turn leds on
                    if (BatteryController::getState() == BatteryController::State_Low ||
                        BatteryController::getState() == BatteryController::State_ChargingLow) {
                        clampColors();
                    }
                    NeoPixel::show(pixels);
                }, nullptr);
            }
        }
    }

    // Convert separate R,G,B to packed value
    uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    void setPowerOn(Timers::DelayedCallback callback, void* parameter) {
        // Are we already on?
        if (powerOn) {
            // Power already on, just proceed.
            callback(parameter);
        } else {
            // Notify clients we're turning led power on
            for (int i = 0; i < ledPowerClients.Count(); ++i) {
                ledPowerClients[i].handler(ledPowerClients[i].token, true);
            }

            // Turn the power on
            NRF_LOG_DEBUG("LED Power On");
            nrf_gpio_pin_set(powerPin);
            powerOn = true;

            // Give enough time for the LEDs to power up (5ms)
            Timers::setDelayedCallback(callback, parameter, 5);
        }
    }

    void setPowerOff() {
        // Already off?
        if (!powerOn)
            return;

        // Turn power off
        NRF_LOG_DEBUG("LED Power Off");
        nrf_gpio_pin_clear(powerPin);
        powerOn = false;

        // Notify clients we're turning led power off
        for (int i = 0; i < ledPowerClients.Count(); ++i) {
            ledPowerClients[i].handler(ledPowerClients[i].token, false);
        }
    }

    uint8_t computeCurrentEstimate() {
        uint32_t nanoAmps = 0; // We don't have anywhere near this precision, it just makes fixed-point computations easier
        int ledOnCount = 0;
        for (int i = 0; i < numLed; ++i) {
            uint32_t color = pixels[i];
            if (color != 0) {
                ledOnCount++;
                // Clamp intensity
                uint8_t r = Utils::getRed(color);
                uint8_t g = Utils::getGreen(color);
                uint8_t b = Utils::getBlue(color);
                nanoAmps += (uint32_t)247000 * 3 + 18200 * (r + g + b);
            }
        }
        if (ledOnCount > 0) {
            nanoAmps += 7100000;
        }

        return (uint8_t)(nanoAmps / 1000000);
    }

}
