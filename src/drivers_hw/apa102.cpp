#include "apa102.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "string.h" // for memset
#include "../utils/utils.h"
#include "../utils/Rainbow.h"
#include "core/delegate_array.h"
#include "../drivers_nrf/log.h"
#include "../drivers_nrf/power_manager.h"

using namespace Config;
using namespace DriversNRF;

#define OFFSET_RED 2
#define OFFSET_GREEN 1
#define OFFSET_BLUE 0

#define MAX_APA102_CLIENTS 2
namespace DriversHW
{
namespace APA102
{
	static uint8_t pixels[MAX_LED_COUNT * 3]; // LED RGB values (3 bytes ea.)
	static uint8_t numLEDs;
	static uint8_t dataPin;
	static uint8_t clockPin;

	void init() {

		// Cache configuration data
		auto board = BoardManager::getBoard();
		dataPin = board->ledDataPin;
		clockPin = board->ledClockPin;
		numLEDs = board->ledCount;
		clear();

		// Initialize the pins
		nrf_gpio_cfg_output(dataPin);
		nrf_gpio_cfg_output(clockPin);

		nrf_gpio_pin_clear(dataPin);
		nrf_gpio_pin_clear(clockPin);


		#if DICE_SELFTEST && APA102_SELFTEST
		selfTest();
		#endif

		NRF_LOG_INFO("APA102 Initialized");
	}

	void clear() {
		memset(pixels, 0, numLEDs * 3);
	}

	#define spi_out(n) swSpiOut(n)

	void swSpiOut(uint8_t n) { // Bitbang SPI write
		for (uint8_t i = 8; i--; n <<= 1) {
			if (n & 0x80) {
				nrf_gpio_pin_set(dataPin);
			} else {
				nrf_gpio_pin_clear(dataPin);
			}
			nrf_gpio_pin_set(clockPin);
			nrf_gpio_pin_clear(clockPin);
		}
	}

	void show(uint32_t* colors) {

		for (int i = 0; i < numLEDs; ++i) {
			uint32_t c = colors[i];
			uint8_t *p = &pixels[i * 3];
			p[OFFSET_RED] = (uint8_t)(c >> 16);
			p[OFFSET_GREEN] = (uint8_t)(c >> 8);
			p[OFFSET_BLUE] = (uint8_t)c;
		}

		uint8_t *ptr = pixels;            // -> LED data
		uint16_t n = numLEDs;              // Counter

		for (int i = 0; i < 4; i++) {
			swSpiOut(0);    // Start-frame marker
		}
		do {                               // For each pixel...
			swSpiOut(0xFF);                //  Pixel start
			for (int i = 0; i < 3; i++) {
				uint8_t comp = *ptr;
				swSpiOut(comp); // R,G,B
				ptr++;
			}
		} while (--n);
		for (int i = 0; i < ((numLEDs + 15) / 16); i++) {
			swSpiOut(0xFF); // End-frame marker (see note above)
		}

        nrf_gpio_pin_clear(dataPin);
        nrf_gpio_pin_clear(clockPin);
	}

	#if DICE_SELFTEST && APA102_SELFTEST
	void selfTest() {

        NRF_LOG_INFO("Turning LEDs On, press any key to stop");
		for (int i = 0; i < numLEDs; ++i) {
			int phase = 255 * i / numLEDs;
			uint32_t color = Rainbow::wheel(phase);
			setPixelColor(i, color);
		}
		show();
		int loop = 0;
        while (true) {
			// PowerManager::feed();
            // PowerManager::update();

			for (int i = 0; i < numLEDs; ++i) {
				int phase = ((int)(255 * i / numLEDs) + loop) % 256;
				uint32_t color = Rainbow::wheel(phase);
				setPixelColor(i, color);
			}
			show();

			loop++;
			nrf_delay_ms(100);
        }
		Log::getKey();
        NRF_LOG_INFO("Turning LEDs Off!");
    	clear();
		show();
	}
	#endif
}
}
