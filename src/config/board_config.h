#pragma once

#define MAX_LED_COUNT 21
#define MAX_BOARD_RESISTOR_VALUES 2

#include "stddef.h"
#include "core/float3.h"
#include "stdint.h"

namespace Config
{
    enum class AccelerometerModel
    {
        LID2DE12,
        KXTJ3_1057,
        QMA7981,
        MXC4005XC,
    };

    enum class LEDModel
    {
        APA102,
        NEOPIXEL_RGB,
        NEOPIXEL_GRB
    };

    namespace BoardManager
    {
        struct Board
        {
            // Measuring board type
            int boardResistorValue;

            // Talking to LEDs
            uint32_t ledDataPin;
            uint32_t ledClockPin;
            uint32_t ledPowerPin;

            // I2C Pins for accelerometer
            uint32_t i2cDataPin;
            uint32_t i2cClockPin;
            uint32_t accInterruptPin;

            // Power Management pins
            uint32_t chargingStatePin;
            uint32_t coilSensePin;
            uint32_t vbatSensePin;
            uint32_t vledSensePin;

            // Magnet pin
            uint32_t magnetPin;

            // Accelerometer component
            AccelerometerModel accModel;
            LEDModel ledModel;

            // LED config
            int ledCount;

            // Name of the board
            const char* name;
        };

        void init();
        const Board* getBoard();
    }
}

