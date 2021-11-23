#include "leds.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "core/delegate_array.h"
#include "drivers_hw/apa102.h"
#include "drivers_hw/neopixel.h"
#include "../drivers_nrf/log.h"

#include "string.h" // for memset

using namespace Config;
using namespace DriversHW;

#define OFFSET_RED 2
#define OFFSET_GREEN 1
#define OFFSET_BLUE 0

#define MAX_APA102_CLIENTS 2
namespace Modules
{
namespace LEDs
{
	DelegateArray<LEDClientMethod, MAX_APA102_CLIENTS> ledPowerClients;
	static uint8_t powerPin;
    static uint8_t numLed;
    uint32_t pixels[MAX_LED_COUNT];

    void show();

	void init() {
        // Initialize Power pin
		auto board = BoardManager::getBoard();
		powerPin = board->ledPowerPin;

		nrf_gpio_cfg_output(powerPin);
		nrf_gpio_pin_clear(powerPin);

        // Initialize our color array
        memset(pixels, 0, MAX_LED_COUNT * sizeof(uint32_t));
        numLed = board->ledCount;

        NRF_LOG_INFO("LEDs Initialized with powerPin=%d", (int)powerPin);
    }

    void clear() {
        memset(pixels, 0, MAX_LED_COUNT * sizeof(uint32_t));
        show();
    }

    void setPixelColor(uint16_t n, uint32_t c) {
        pixels[n] = c;
        show();
    }

    void setAll(uint32_t c) {
        for (int i = 0; i < numLed; ++i) {
            pixels[i] = c;
        }
        show();
    }

    void setPixelColors(int* indices, uint32_t* colors, int count) {
        for (int i = 0; i < count; ++i) {
            pixels[indices[i]] = colors[i];
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

	void prepare(void) {
		// Sets the power pin on, but doesn't write any data
		// Turn power on so we display something!!!
		if (nrf_gpio_pin_out_read(powerPin) == 0) {

			// Notify clients we're turning led power on
			for (int i = 0; i < ledPowerClients.Count(); ++i) {
				ledPowerClients[i].handler(ledPowerClients[i].token, true);
			}

			nrf_gpio_pin_set(powerPin);
			nrf_delay_ms(2); // Anything less than 2ms before toggling data/clk lines will cause artifacts
            NRF_LOG_INFO("LED Power On");
		}
	}

    bool isPixelDataZero() {
        bool allOff = true;
        for (int i = 0; i < numLed; ++i) {
            if (pixels[i] != 0) {
                allOff = false;
                break;
            }
        }
        return allOff;
    }

    void show() {
		// Are all the physical leds already all off?
		bool powerOff = nrf_gpio_pin_out_read(powerPin) == 0;

		// Do we want all the leds to be off?
        bool allOff = isPixelDataZero();

		if (powerOff && allOff) {
            NRF_LOG_DEBUG("LED Power Already Off");

			// Displaying all black and we've already turned every led off
			return;
		}

		// Turn power on so we display something!!!
		prepare();

        switch (Config::BoardManager::getBoard()->ledModel) {
            case Config::LEDModel::APA102:
                APA102::show(pixels);
                break;
             case Config::LEDModel::NEOPIXEL_RGB:
             case Config::LEDModel::NEOPIXEL_GRB:
                NeoPixel::show(pixels);
                break;
        }

		if (allOff) {
			// Turn power off too
            NRF_LOG_INFO("LED Power Off");
			nrf_gpio_pin_clear(powerPin);

			// Notify clients we're turning led power off
			for (int i = 0; i < ledPowerClients.Count(); ++i) {
				ledPowerClients[i].handler(ledPowerClients[i].token, false);
			}
		}
    }


	// Convert separate R,G,B to packed value
	uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
		return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
	}
}
}