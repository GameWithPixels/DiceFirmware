#include "board_config.h"
#include "drivers_nrf/a2d.h"
#include "nrf_gpio.h"
#include "nrf_saadc.h"
#include "nrf_log.h"
#include "settings.h"

#define BOARD_DETECT_DRIVE_PIN 25
#define BOARD_DETECT_RESISTOR 100000 // 100k

namespace Config
{
namespace BoardManager
{
        const uint8_t sixSidedRemap[] = {
            // FIXME!!!
            0, 1, 2, 3, 4, 5,
            1, 2, 3, 4, 5, 0,
            2, 3, 4, 5, 0, 1,
            3, 4, 5, 0, 1, 2,
            4, 5, 0, 1, 2, 3, 
            5, 0, 1, 2, 3, 4, 
        };

        const Core::float3 sixSidedNormals[] = {
            { 0, -1,  0},
            { 0,  0,  1},
            { 1,  0,  0},
            {-1,  0,  0},
            { 0,  0, -1},
            { 0,  1,  0}
        };

        const uint8_t sixSidedFaceToLedLookup[] = {
            1, 4, 0, 3, 2, 5,
        };

        // const uint8_t twentySidedRemap[] = {
        const uint8_t twentySidedRemap[] = {
            19,	18,	11,	10,	5,	4,	7,	6,	17,	16,	3,	2,	13,	12,	15,	14,	9,	8,	1,	0,
            18,	11,	5,	4,	6,	3,	2,	19,	10,	7,	12,	9,	0,	17,	16,	13,	15,	14,	8,	1,
            17,	15,	14,	8,	13,	0,	1,	16,	12,	9,	10,	7,	3,	18,	19,	6,	11,	5,	4,	2,
            16,	17,	15,	12,	14,	8,	9,	13,	18,	19,	0,	1,	6,	10,	11,	5,	7,	4,	2,	3,
            15,	14,	13,	0,	16,	10,	7,	17,	8,	1,	18,	11,	2,	12,	9,	3,	19,	6,	5,	4,
            14,	15,	12,	19,	9,	3,	6,	8,	17,	18,	1,	2,	11,	13,	16,	10,	0,	7,	4,	5,
            13,	16,	17,	18,	15,	12,	19,	14,	10,	11,	8,	9,	5,	0,	7,	4,	1,	2,	3,	6,
            12,	19,	6,	5,	3,	2,	4,	9,	18,	11,	8,	1,	10,	15,	17,	16,	14,	13,	0,	7,
            11,	5,	6,	3,	19,	12,	9,	18,	4,	2,	17,	15,	1,	10,	7,	0,	16,	13,	14,	8,
            10,	11,	18,	19,	17,	15,	12,	16,	5,	6,	13,	14,	3,	7,	4,	2,	0,	1,	8,	9,
            9,	12,	19,	18,	6,	5,	11,	3,	15,	17,	2,	4,	16,	8,	14,	13,	1,	0,	7,	10,
            8,	14,	15,	17,	12,	19,	18,	9,	13,	16,	3,	6,	10,	1,	0,	7,	2,	4,	5,	11,
            7,	0,	1,	8,	2,	3,	9,	4,	13,	14,	5,	6,	15,	10,	16,	17,	11,	18,	19,	12,
            6,	3,	9,	8,	12,	15,	14,	19,	2,	1,	18,	17,	0,	5,	4,	7,	11,	10,	16,	13,
            5,	6,	19,	12,	18,	17,	15,	11,	3,	9,	10,	16,	8,	4,	2,	1,	7,	0,	13,	14,
            4,	7,	0,	13,	1,	8,	14,	2,	10,	16,	3,	9,	17,	5,	11,	18,	6,	19,	12,	15,
            3,	9,	12,	15,	19,	18,	17,	6,	8,	14,	5,	11,	13,	2,	1,	0,	4,	7,	10,	16,
            2,	4,	7,	10,	0,	13,	16,	1,	5,	11,	8,	14,	18,	3,	6,	19,	9,	12,	15,	17,
            1,	2,	4,	5,	7,	10,	11,	0,	3,	6,	13,	16,	19,	8,	9,	12,	14,	15,	17,	18,
            0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,	16,	17,	18,	19,
        };

        const uint8_t twentySidedFaceToLedLookup[] = {
        //   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  // Face number (face index+1)
        //   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  // Face index
             9,  5, 18,  3,  0, 12,  8, 15, 11, 16,  2,  6,  1, 13,  7, 19, 17,  4, 10, 14
        // Old Molds:
        //   9, 13,  7, 19, 11, 16,  1,  5, 17,  4, 18,  3, 10, 15,  2,  6,  0, 12,  8, 14
        };

        // LED number
        //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
        //  4, 12, 10,  3, 17,  1, 11, 14,  6,  0, 18,  8,  5, 13, 19,  7,  9, 16,  2, 15

        const Core::float3 twentySidedNormals[] = {
            {-0.1273862f,  0.3333025f,  0.9341605f},
            { 0.7453963f, -0.3333219f, -0.5773357f},
            {-0.8726854f, -0.3333218f,  0.3568645f},
            { 0.3333614f,  0.7453930f, -0.5774010f},
            { 0.8726999f,  0.3333025f,  0.3567604f},
            {-0.7453431f,  0.3333741f, -0.5773722f},
            { 0.1273475f, -0.3333741f,  0.9341723f},
            {-0.3333083f, -0.7453408f, -0.5773069f},
            {-0.6667246f,  0.7453931f, -0.0000000f},
            { 0.0000000f, -1.0000000f, -0.0000000f},
            { 0.0000000f,  1.0000000f, -0.0000000f},
            { 0.6667246f, -0.7453931f, -0.0000000f},
            { 0.3333083f,  0.7453408f,  0.5773069f},
            {-0.1273475f,  0.3333741f, -0.9341723f},
            { 0.7453431f, -0.3333741f,  0.5773722f},
            {-0.8726999f, -0.3333025f, -0.3567604f},
            {-0.3331230f, -0.7450288f,  0.5778139f},
            { 0.8726854f,  0.3333218f, -0.3568645f},
            {-0.7453963f,  0.3333219f,  0.5773357f},
            { 0.1273862f, -0.3333025f, -0.9341605f},
        };

        // const uint8_t twelveSidedRemap[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        //     1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0,
        //     2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 
        //     3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 
        //     4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 
        //     5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 
        //     6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 
        //     7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 
        //     8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 
        //     9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 
        //     10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        //     11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
        // };

        // const Core::float3 twelveSidedNormals[] = {
        //     // FIXME!!!
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        //     { 1,  0,  0},
        //     {-1,  0,  0},
        //     { 0,  0, -1},
        //     { 0,  1,  0},
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        //     { 1,  0,  0},
        //     {-1,  0,  0},
        //     { 0,  0, -1},
        //     { 0,  1,  0},
        // };

        // const uint8_t twelveSidedFaceToLedLookup[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
        // };


        // const uint8_t tenSidedRemap[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        //     1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
        //     2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 
        //     3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 
        //     4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 
        //     5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 
        //     6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 
        //     7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 
        //     8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 
        //     9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 
        // };

        // const Core::float3 tenSidedNormals[] = {
        //     // FIXME!!!
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        //     { 1,  0,  0},
        //     {-1,  0,  0},
        //     { 0,  0, -1},
        //     { 0,  1,  0},
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        //     { 1,  0,  0},
        //     {-1,  0,  0},
        // };

        // const uint8_t tenSidedFaceToLedLookup[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        // };


        // const uint8_t eightSidedRemap[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7,
        //     1, 2, 3, 4, 5, 6, 7, 0, 
        //     2, 3, 4, 5, 6, 7, 0, 1, 
        //     3, 4, 5, 6, 7, 0, 1, 2, 
        //     4, 5, 6, 7, 0, 1, 2, 3, 
        //     5, 6, 7, 0, 1, 2, 3, 4, 
        //     6, 7, 0, 1, 2, 3, 4, 5, 
        //     7, 0, 1, 2, 3, 4, 5, 6, 
        // };

        // const Core::float3 eightSidedNormals[] = {
        //     // FIXME!!!
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        //     { 1,  0,  0},
        //     {-1,  0,  0},
        //     { 0,  0, -1},
        //     { 0,  1,  0},
        //     { 0, -1,  0},
        //     { 0,  0,  1},
        // };

        // const uint8_t eightSidedFaceToLedLookup[] = {
        //     // FIXME!!!
        //     0, 1, 2, 3, 4, 5, 6, 7
        // };


    // Array of possible circuit boards configs
    // Note that the boards MUST be sorted in order of INCREASING resistor value
    // for the init method to properly find the correct board config.
    static const Board boards[] = {
        // Dev Board
        // {
        //     .boardResistorValueInKOhms = 0,
        //     .ledDataPin = 6,
        //     .ledClockPin = 5,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 15,
        //     .accInterruptPin = 16,
        //     .chargingStatePin = 0xFF,
        //     .coilSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN2,
        //     .vledSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 21,
        //     .name = "Dev",
        // },
        // D20Board
        // {
        //     .boardResistorValueInKOhms = 20, // 20k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 4,
        //     .ledPowerPin = 0,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 0xFF,
        //     .coilSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 20,
        //     .name = "D20v1",
        // },
        // D20BoardV5
        // {
        //     .boardResistorValueInKOhms = 33, // 33k or 56k Resistor
        //     .ledDataPin = 0,
        //     .ledClockPin = 1,
        //     .ledPowerPin = 10,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 9,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 20,
        //     .name = "D20v5",
        // },
        // D6Board
        // {
        //     .boardResistorValueInKOhms = 47, // 47k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 4,
        //     .ledPowerPin = 0,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 0xFF,
        //     .coilSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_DISABLED,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 6,
        //     .name = "D6v1",
        // },
        // D20BoardV5
        // {
        //     .boardResistorValueInKOhms = 56, // 33k or 56k Resistor
        //     .ledDataPin = 0,
        //     .ledClockPin = 1,
        //     .ledPowerPin = 10,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 9,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 20,
        //     .name = "D20v5",
        // },
        // D20BoardV8
        // {
        //     .boardResistorValueInKOhms = 68, // 68k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 0,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::APA102,
        //     .ledCount = 20,
        //     .name = "D20v8",
        // },
        // D20BoardV9Alt1
        // {
        //     .boardResistorValueInKOhms = 75, // 75k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 12,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::NEOPIXEL_RGB,
        //     .ledCount = 20,
        //     .name = "D20v9.1",
        // },
        // D20BoardV9Alt2
        // {
        //     .boardResistorValueInKOhms = 77, // 71.5k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 15,
        //     .accInterruptPin = 12,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::KXTJ3_1057,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 20,
        //     .name = "D20v9.2",
        // },
        // D20BoardV9Alt3
        // {
        //     .boardResistorValueInKOhms = 79, // 78.7k Resistor
        //     .ledDataPin =  1,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 12,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::LID2DE12,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 20,
        //     .name = "D20v9.3",
        // },
        // D20BoardV9Alt4
        // {
        //     .boardResistorValueInKOhms = 79, // 78.7k Resistor
        //     .ledDataPin = 1,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 9,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 12,
        //     .accInterruptPin = 15,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN6,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN2,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::MXC4005XC,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 20,
        //     .name = "D20v9.4",
        // },
        //D20BoardV10
        {
            .boardResistorValueInKOhms = 82, // 82.5k Resistor, at VCC = 3.1V, this means 3.1V * 82.5k / 182.5k = 1.4V
            .ledDataPin = 1,
            .ledClockPin = 0xFF,
            .ledPowerPin = 9,
            .ledReturnPin = 0xFF,
            .i2cDataPin = 14,
            .i2cClockPin = 10,
            .accInterruptPin = 15,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN3,
            .vbatSensePin = NRF_SAADC_INPUT_AIN6,
            .vledSensePin = NRF_SAADC_INPUT_AIN2,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::MXC4005XC,
            .ledModel = LEDModel::NEOPIXEL_GRB,
            .ledCount = 20,
            .model = BoardModel::D20BoardV10,
            .layout = {
                .baseNormals = twentySidedNormals,
                .faceRemap = twentySidedRemap,
                .faceToLedLookup = twentySidedFaceToLedLookup,
            },
            .name = "D20v10",
        },
        //D20BoardV11
        {
            .boardResistorValueInKOhms = 100, // 100.0k Resistor, at VCC = 3.1V, this means 3.1V * 100k / 200k = 1.55V
            .ledDataPin = 6,
            .ledClockPin = 0xFF,
            .ledPowerPin = 0,
            .ledReturnPin = 0xFF,
            .i2cDataPin = 15,
            .i2cClockPin = 14,
            .accInterruptPin = 12,
            .chargingStatePin = 1,
            .coilSensePin = NRF_SAADC_INPUT_AIN2,
            .vbatSensePin = NRF_SAADC_INPUT_AIN3,
            .vledSensePin = NRF_SAADC_INPUT_AIN6,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::KXTJ3_1057,
            .ledModel = LEDModel::NEOPIXEL_GRB,
            .ledCount = 20,
            .model = BoardModel::D20BoardV11,
            .layout = {
                .baseNormals = twentySidedNormals,
                .faceRemap = twentySidedRemap,
                .faceToLedLookup = twentySidedFaceToLedLookup,
            },
            .name = "D20v11",
        },
        //D6BoardV2
        {
            .boardResistorValueInKOhms = 120, // 120.0k Resistor, at VCC = 3.1V, this means 3.1V * 120k / 220k = 1.69V
            .ledDataPin = 1,
            .ledClockPin = 0xFF,
            .ledPowerPin = 0,
            .ledReturnPin = 0xFF,
            .i2cDataPin = 14,
            .i2cClockPin = 15,
            .accInterruptPin = 12,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN2,
            .vbatSensePin = NRF_SAADC_INPUT_AIN3,
            .vledSensePin = NRF_SAADC_INPUT_AIN6,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::KXTJ3_1057,
            .ledModel = LEDModel::NEOPIXEL_GRB,
            .ledCount = 6,
            .model = BoardModel::D6BoardV2,
            .layout = {
                .baseNormals = sixSidedNormals,
                .faceRemap = sixSidedRemap,
                .faceToLedLookup = sixSidedFaceToLedLookup,
            },
            .name = "D6v2",
        },
        // //PD6BoardV1
        // {
        //     .boardResistorValueInKOhms = 150, // 150.0k Resistor, at VCC = 3.1V, this means 3.1V * 150k / 250k = 1.86V
        //     .ledDataPin = 1,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 0,
        //     .ledReturnPin = 0xFF,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 15,
        //     .accInterruptPin = 12,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN2,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN6,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::KXTJ3_1057,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 21,
        //     .model = BoardModel::PD6BoardV1,
        //     .layout = {
        //         .baseNormals = sixSidedNormals,
        //         .faceRemap = sixSidedRemap,
        //         .faceToLedLookup = sixSidedFaceToLedLookup,
        //     },
        //     .name = "PD6v1",
        // },
        // //D12BoardV1
        // {
        //     .boardResistorValueInKOhms = 180, // 180.0k Resistor, at VCC = 3.1V, this means 3.1V * 180k / 280k = 1.99V
        //     .ledDataPin = 6,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 0,
        //     .ledReturnPin = 0xFF,
        //     .i2cDataPin = 15,
        //     .i2cClockPin = 14,
        //     .accInterruptPin = 12,
        //     .chargingStatePin = 1,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN2,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN6,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::KXTJ3_1057,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 12,
        //     .model = BoardModel::D12BoardV1,
        //     .layout = {
        //         .baseNormals = twelveSidedNormals,
        //         .faceRemap = twelveSidedRemap,
        //         .faceToLedLookup = twelveSidedFaceToLedLookup,
        //     },
        //     .name = "D12v1",
        // },
        // // D10BoardV1
        // {
        //     .boardResistorValueInKOhms = 270, // 270.0k Resistor, at VCC = 3.1V, this means 3.1V * 270k / 370k = 2.26V
        //     .ledDataPin = 0,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 10,
        //     .ledReturnPin = 1,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 15,
        //     .accInterruptPin = 12,
        //     .chargingStatePin = 6,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN2,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN6,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::KXTJ3_1057,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 10,
        //     .model = BoardModel::D10BoardV1,
        //     .layout = {
        //         .baseNormals = tenSidedNormals,
        //         .faceRemap = tenSidedRemap,
        //         .faceToLedLookup = tenSidedFaceToLedLookup,
        //     },
        //     .name = "D10v1",
        // },
        // //D8BoardV1
        // {
        //     .boardResistorValueInKOhms = 390, // 390.0k Resistor, at VCC = 3.1V, this means 3.1V * 390k / 390k = 2.47V
        //     .ledDataPin = 9,
        //     .ledClockPin = 0xFF,
        //     .ledPowerPin = 6,
        //     .ledReturnPin = 10,
        //     .i2cDataPin = 14,
        //     .i2cClockPin = 15,
        //     .accInterruptPin = 12,
        //     .chargingStatePin = 0,
        //     .coilSensePin = NRF_SAADC_INPUT_AIN2,
        //     .vbatSensePin = NRF_SAADC_INPUT_AIN3,
        //     .vledSensePin = NRF_SAADC_INPUT_AIN6,
        //     .magnetPin = 0xFF,
        //     .accModel = AccelerometerModel::KXTJ3_1057,
        //     .ledModel = LEDModel::NEOPIXEL_GRB,
        //     .ledCount = 8,
        //     .model = BoardModel::D8BoardV1,
        //     .layout = {
        //         .baseNormals = eightSidedNormals,
        //         .faceRemap = eightSidedRemap,
        //         .faceToLedLookup = eightSidedFaceToLedLookup,
        //     },
        //     .name = "D8v1",
        // },
    };

    // 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20   <-- prev
    // 01 12 18 08 10 19 04 06 05 07 14 16 15 17 02 11 13 03 09 20   <-- next


    // The board we're currently using
    static const Board* currentBoard = nullptr;

    void init() {
        // Sample adc board pin
        nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
        nrf_gpio_pin_set(BOARD_DETECT_DRIVE_PIN);

        float vboard = DriversNRF::A2D::readVBoard();

        // Now that we're done reading, we can turn off the drive pin
        nrf_gpio_cfg_default(BOARD_DETECT_DRIVE_PIN);

        // Do some computation to figure out which variant we're working with!
        // D20v3 board uses 20k over 100k voltage divider
        // i.e. the voltage read should be 3.1V * 20k / 120k = 0.55V
        // The D6v2 board uses 47k over 100k, i.e. 1.05V
        // The D20v2 board should read 0 (unconnected)
        // So we can allow a decent
        //const float vdd = DriversNRF::A2D::readVDD();
        const float vdd = DriversNRF::A2D::readVDD();

        // Compute board voltages
        const int boardCount = sizeof(boards) / sizeof(boards[0]);
        float boardVoltages[boardCount];
        for (int i = 0; i < boardCount; ++i) {
            boardVoltages[i] = (vdd * boards[i].boardResistorValueInKOhms * 1000) / (float)(BOARD_DETECT_RESISTOR + boards[i].boardResistorValueInKOhms * 1000);
            NRF_LOG_INFO("%s: voltage:" NRF_LOG_FLOAT_MARKER, boards[i].name, NRF_LOG_FLOAT(boardVoltages[i]));
        }
        float midpointVoltages[boardCount-1];
        for (int i = 0; i < boardCount-1; ++i) {
            midpointVoltages[i] = (boardVoltages[i] + boardVoltages[i+1]) * 0.5f;
        }

        // Find the first midpoint voltage that is above the measured voltage
        int boardIndex = 0;
        for (; boardIndex < boardCount-1; ++boardIndex) {
            if (midpointVoltages[boardIndex] > vboard)
            break;
        }
        currentBoard = &(boards[boardIndex]);
        NRF_LOG_INFO("Board is %s, boardIdVoltage=" NRF_LOG_FLOAT_MARKER, currentBoard->name, NRF_LOG_FLOAT(vboard));
    }

    const Board* getBoard() {
        return currentBoard;
    }


}
}
