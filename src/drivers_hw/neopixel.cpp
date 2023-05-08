/*
 * NeoPixel WS2812B Driver for nRF52 series.
 *  (c) 2019, Aquamarine Networks.
 */


#include "neopixel.h"
#include "nrf_drv_pwm.h"
#include "nrf_delay.h"
#include "settings.h"
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

namespace DriversHW
{
    namespace NeoPixel
    {
        static nrf_drv_pwm_t m_pwm0;
        static nrf_pwm_values_common_t pwm_sequence_values[MAX_COUNT * NEOPIXEL_BYTES + 1];
        static uint8_t numLEDs;
        static uint8_t dataPin;

        struct component
        {
            uint32_t colorMask;
            uint16_t colorShift;
            uint16_t pwmSequenceOffset;
        };

        void writeColor(uint32_t color, uint32_t ledIndex) {

            // Reorder the color bytes to match the hardware
            int pwm_baseIndex = 24 * ledIndex;
            for (int i = 0; i < 8; ++i) {
                pwm_sequence_values[pwm_baseIndex + 16 + i] = ((color & 0x000080) == 0 ? DUTY0 : DUTY1) | 0x8000;
                pwm_sequence_values[pwm_baseIndex +  8 + i] = ((color & 0x800000) == 0 ? DUTY0 : DUTY1) | 0x8000;
                pwm_sequence_values[pwm_baseIndex      + i] = ((color & 0x008000) == 0 ? DUTY0 : DUTY1) | 0x8000;
                color <<= 1;
            }
        }

        void pwm_handler(nrf_drv_pwm_evt_type_t event_type) {
            // Nothing
        }

        void init() {

            m_pwm0.p_registers  = NRFX_CONCAT_2(NRF_PWM, NEOPIXEL_INSTANCE);
            m_pwm0.drv_inst_idx = NRFX_CONCAT_3(NRFX_PWM, NEOPIXEL_INSTANCE, _INST_IDX);

            // Cache configuration data
            const Board* board = Config::BoardManager::getBoard();
            dataPin = board->ledDataPin;
            numLEDs = board->ledCount;
            
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

            NRF_LOG_DEBUG("Neopixel init");
        }

        void uninit(void) {
            nrf_drv_pwm_uninit(&m_pwm0);
        }


        void clear() {
            for (uint32_t led = 0; led < numLEDs; led++) {
                writeColor(0, led);
            }
        }

        void show(uint32_t* colors) {
            for (int i = 0; i < numLEDs; i++) {
                writeColor(colors[i], i);
            }
            // write the termination word
            pwm_sequence_values[numLEDs * NEOPIXEL_BYTES] = 0x8000;

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

        void testLEDReturn() {
            // Forces LEDs to forward color values past the last one so we can detect it
            for (int i = 0; i < numLEDs+1; i++) {
                writeColor(0, i);
            }
            // write the termination word
            pwm_sequence_values[(numLEDs+1) * NEOPIXEL_BYTES] = 0x8000;

            nrf_pwm_sequence_t const seq0 =
                {
                    .values = {
                        .p_common = pwm_sequence_values,
                    },
                    .length = (uint16_t)(NEOPIXEL_BYTES * (numLEDs + 1) + 1),
                    .repeats = 0,
                    .end_delay = 0};

            (void)nrf_drv_pwm_simple_playback(&m_pwm0, &seq0, 1, NRF_DRV_PWM_FLAG_STOP);
        }
    }
}