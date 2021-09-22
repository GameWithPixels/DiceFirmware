/*
 * NeoPixel WS2812B Driver for nRF52 series.
 *  (c) 2019, Aquamarine Networks.
 */


#include "neopixel.h"
#include "nrf_gpio.h"
#include "nrf_drv_pwm.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "../drivers_nrf/log.h"

using namespace Config;

// change this for your settings
#define NEOPIXEL_INSTANCE 0

#define NEOPIXEL_BYTES 24
#define CLOCK NRF_PWM_CLK_16MHz
#define TOP 20
#define DUTY0 6
#define DUTY1 13

#define NEOPIXEL_GRB(green,red,blue) ( ((uint8_t)(blue)) | (((uint8_t)(red)) << 8) | (((uint8_t)(green)) << 16))

namespace DriversHW
{
    namespace NeoPixel
    {
        static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(NEOPIXEL_INSTANCE);
        static nrf_pwm_values_common_t pwm_sequence_values[MAX_LED_COUNT * NEOPIXEL_BYTES + 1];

        typedef  void (*neopixel_handler_t)(void);
        static neopixel_handler_t m_handler = NULL;

        static uint8_t numLEDs;
        static uint8_t dataPin;
        static Config::LEDModel ledModel;

        struct component
        {
            uint32_t colorMask;
            uint16_t colorShift;
            uint16_t pwmSequenceOffset;
        };

        static const component rgbComponents[] = {
            // Red
            {
                .colorMask = 0xFF0000, // mask in source color
                .colorShift = 16, // bit offset in source color
                .pwmSequenceOffset = 0 // short offset in pwm sequence (i.e. bit offset in wire data)
            },
            // Green
            {
                .colorMask = 0xFF00,
                .colorShift = 8, // bits
                .pwmSequenceOffset = 8
            },
            // Blue
            {
                .colorMask = 0xFF,
                .colorShift = 0, // bits
                .pwmSequenceOffset = 16
            },
        };

        static const component grbComponents[] = {
            // Red
            {
                .colorMask = 0xFF0000,
                .colorShift = 16, // bits
                .pwmSequenceOffset = 8
            },
            // Green
            {
                .colorMask = 0xFF00,
                .colorShift = 8, // bits
                .pwmSequenceOffset = 0
            },
            // Blue
            {
                .colorMask = 0xFF,
                .colorShift = 0, // bits
                .pwmSequenceOffset = 16
            },
        };


        void writeColor(uint32_t color, uint32_t ledIndex);

        void pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
            if (event_type == NRF_DRV_PWM_EVT_FINISHED) {
                if ( m_handler != NULL ) {
                    m_handler();
                }
            }
        }

        void init() {

            // Cache configuration data
            auto board = BoardManager::getBoard();
            dataPin = board->ledDataPin;
            numLEDs = board->ledCount;
            ledModel = board->ledModel;

            m_handler = nullptr;
            nrf_drv_pwm_config_t const config0 =
                {
                    .output_pins =
                        {
                            dataPin,                  // channel 0
                            NRF_DRV_PWM_PIN_NOT_USED, // channel 1
                            NRF_DRV_PWM_PIN_NOT_USED, // channel 2
                            NRF_DRV_PWM_PIN_NOT_USED  // channel 3
                        },
                    .irq_priority = APP_IRQ_PRIORITY_LOWEST,
                    .base_clock = CLOCK,
                    .count_mode = NRF_PWM_MODE_UP,
                    .top_value = TOP,
                    .load_mode = NRF_PWM_LOAD_COMMON,
                    .step_mode = NRF_PWM_STEP_AUTO};
            APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, pwm_handler));

            // Pre-write the termination word
            pwm_sequence_values[numLEDs * NEOPIXEL_BYTES] = 0x8000;

            NRF_LOG_INFO("Neopixel Initialized");
        }

        void uninit(void) {
            nrf_drv_pwm_uninit(&m_pwm0);
        }


        void clear() {
            for (uint32_t led = 0; led < numLEDs; led++) {
                writeColor(0, led);
            }
        }

        void writeColor(uint32_t color, uint32_t ledIndex) {

            const component* components = ledModel == Config::LEDModel::NEOPIXEL_RGB ? rgbComponents : grbComponents;
            for (int comp = 0; comp < 3; ++comp) {
                uint8_t color_comp = (color & components[comp].colorMask) >> components[comp].colorShift;
                uint16_t offset = components[comp].pwmSequenceOffset;
                for (uint8_t i = 0; i < 8; ++i) {
                    uint16_t value = (color_comp & 0x80) == 0 ? DUTY0 : DUTY1;
                    pwm_sequence_values[ledIndex * NEOPIXEL_BYTES + offset + i] = value | 0x8000;
                    color_comp <<= 1;
                }
            }
        }

        void show(uint32_t* colors) {
            for (int i = 0; i < numLEDs; i++) {
                writeColor(colors[i], i);
            }

            nrf_pwm_sequence_t const seq0 =
                {
                    .values = {
                        .p_common = pwm_sequence_values,
                    },
                    .length = (uint16_t)(NEOPIXEL_BYTES * numLEDs + 1),
                    .repeats = 0,
                    .end_delay = 0};

            (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1, NRF_DRV_PWM_FLAG_STOP);
        }
    }
}