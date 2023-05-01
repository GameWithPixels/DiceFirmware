#include "neopixel_bitbang.h"
#include "compiler_abstraction.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define MAX_COUNT 21

void NeopixelBitBangSetLEDs(uint32_t* colors, int count, uint8_t dataPin, uint8_t powerPin) {

    // Shenanigans to toggle the data pin faster
    uint32_t dataPin_u32 = dataPin;
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&dataPin_u32);
    uint32_t dataPinMask = 1UL << dataPin;

    // Make sure pins are setup correctly, just in case
    nrf_gpio_cfg_output(dataPin);
    nrf_gpio_cfg_output(powerPin);

    // Should we turn power off?
    bool isDataZero = true;
    for (int i = 0; i < count; ++i) {
        if (colors[i] != 0) {
            isDataZero = false;
            break;
        }
    }

    if (isDataZero) {
        // Turn power off
        nrf_gpio_pin_clear(powerPin);
        nrf_gpio_pin_clear(dataPin);
    } else {
        // Turn power on
        nrf_gpio_pin_set(powerPin);
        nrf_gpio_pin_clear(dataPin);

        // Wait for LEDs to start up
        nrf_delay_ms(5);

        // Reorder the color bytes to match the hardware
        uint8_t frameData[MAX_COUNT * 3];
        int j = 0;
        for (; j < count; j++) {
            frameData[j * 3 + 2] = (colors[j] & 0x0000FF) >> 0;
            frameData[j * 3 + 1] = (colors[j] & 0xFF0000) >> 16;
            frameData[j * 3 + 0] = (colors[j] & 0x00FF00) >> 8;
        }
        for (; j < MAX_COUNT; ++j) {
            frameData[j * 3 + 2] = 0;
            frameData[j * 3 + 1] = 0;
            frameData[j * 3 + 0] = 0;
        }

        // This loop is structured strangely on purpose to minimize the amount of
        // time spent in the loop logic, as the timing is critical to get the leds
        // to decode the data correctly.

        // What we are doing is simple in principle: for each bit of the frame data we
        // toggle the data pin high and low:
        // - For a 0 bit, we toggle high for ~300ns and low for ~600ns
        // - For a 1 bit, we toggle high for ~600ns and low for ~300ns
        // but to avoid incorrect timing from loop logic, we do the following:
        // - Make a single loop logic and use math to extract the byte index and bit mask
        // - Try to offload that math to the "long" portion of the high-low cycle.

        uint8_t test = frameData[0] & 0x80;
        int byteCount = 3 * MAX_COUNT * 8 + 1;
        for (int i = 1; i < byteCount; ++i) {
            if (test) {
                // 1 bit, start high for 600ns
                nrf_gpio_port_out_set(reg, dataPinMask);
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); 
                // The math here takes a significant amount of time
                test = frameData[i >> 3] & (0x80 >> (i & 0x07));
                nrf_gpio_port_out_clear(reg, dataPinMask);
                // Loop, minimizing the amount of time spent in the loop and branch tests
            } else {
                // 0 bit, start high for 300ns
                nrf_gpio_port_out_set(reg, dataPinMask);
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP();
                // Then go low for 600ns
                nrf_gpio_port_out_clear(reg, dataPinMask);
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP(); __NOP();
                __NOP(); __NOP(); __NOP();
                // The math here takes a significant amount of time
                test = frameData[i >> 3] & (0x80 >> (i & 0x07));
                // Completing the loop will take the remainder amount of time
            }
        }
        nrf_gpio_port_out_clear(reg, dataPinMask);
    }
}

void NeopixelBitBangSetD20Face20LED(uint32_t color) {
    uint32_t colors[MAX_COUNT];
    for (int i = 0; i < MAX_COUNT; ++i) {
        colors[i] = 0;
    }
    colors[14] = color;
    NeopixelBitBangSetLEDs(colors, MAX_COUNT, 6, 0);
}

