#pragma once

#define MAX_LED_COUNT 21
#define MAX_BOARD_RESISTOR_VALUES 2

#include "stddef.h"
#include "core/float3.h"
#include "stdint.h"

#pragma pack(push, 1)

namespace Config
{
    enum class AccelerometerModel : uint8_t
    {
        LID2DE12,
        KXTJ3_1057,
        QMA7981,
        MXC4005XC,
    };

    enum class LEDModel : uint8_t
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
            uint8_t boardResistorValueInKOhms;

            // Talking to LEDs
            uint8_t ledDataPin;
            uint8_t ledClockPin;
            uint8_t ledPowerPin;

            // I2C Pins for accelerometer
            uint8_t i2cDataPin;
            uint8_t i2cClockPin;
            uint8_t accInterruptPin;

            // Power Management pins
            uint8_t chargingStatePin;
            uint8_t coilSensePin;
            uint8_t vbatSensePin;
            uint8_t vledSensePin;

            // Magnet pin
            uint8_t magnetPin;

            // Accelerometer component
            AccelerometerModel accModel;
            LEDModel ledModel;

            // LED config
            uint8_t ledCount;

            // Name of the board
            const char* name;
        };

        void init();
        const Board* getBoard();
    }
}

#pragma pack(pop)
