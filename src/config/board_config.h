#pragma once

#define MAX_BOARD_RESISTOR_VALUES 2

#include "stddef.h"
#include "core/float3.h"
#include "stdint.h"
#include "dice_variants.h"

#pragma pack(push, 1)

namespace Config
{
    namespace DiceVariants
    {
        struct Layout;
    }

    enum class AccelerometerModel : uint8_t
    {
        LID2DE12,
        KXTJ3_1057,
        QMA7981,
        MXC4005XC,
    };

    enum class LEDModel : uint8_t
    {
        NEOPIXEL_RGB,
        NEOPIXEL_GRB
    };

    enum class BoardModel : uint8_t
    {
        DevBoard = 0,
        D20Board,
        D20BoardV5,
        D6Board,
        D20BoardV6,
        D20BoardV8,
        D20BoardV9Alt1,
        D20BoardV9Alt2,
        D20BoardV9Alt3,
        D20BoardV9Alt4,
        D20BoardV10,
        D20BoardV11,
        D6BoardV2,
        D12BoardV1,
        PD6BoardV1,
        D10BoardV1,
        D8BoardV1,
    };

    namespace BoardManager
    {
        struct Board
        {
            // Measuring board type
            uint16_t boardResistorValueInKOhms;

            // Talking to LEDs
            uint8_t ledDataPin;
            uint8_t ledClockPin;
            uint8_t ledPowerPin;
            uint8_t ledReturnPin;

            // I2C Pins for accelerometer
            uint8_t i2cDataPin;
            uint8_t i2cClockPin;
            uint8_t accInterruptPin;

            // Power Management pins
            uint8_t chargingStatePin;
            uint8_t coilSensePin;
            uint8_t vbatSensePin;
            uint8_t vledSensePin;
            uint8_t progPin;

            // Magnet pin
            uint8_t magnetPin;

            // Accelerometer component
            AccelerometerModel accModel;
            LEDModel ledModel;

            // LED config
            uint8_t ledCount;

            // Board
            BoardModel model;

            // Padding
            uint16_t padding;

            // Die layout information
            DiceVariants::Layout layout;

            // Name of the board
            const char* name;

            uint8_t animIndexToLEDIndex(int animFaceIndex, int remapFace) const;
        };

        void init();
        const Board* getBoard();
    }
}

#pragma pack(pop)
