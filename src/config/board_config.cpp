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
    // Array of possible circuit boards configs
    // Note that the boards MUST be sorted in order of INCREASING resistor value
    // for the init method to properly find the correct board config.
    static const Board boards[] = {
        // Dev Board
        {
            .boardResistorValueInKOhms = 0,
            .ledDataPin = 6,
            .ledClockPin = 5,
            .ledPowerPin = 9,
            .i2cDataPin = 14,
            .i2cClockPin = 15,
            .accInterruptPin = 16,
            .chargingStatePin = 0xFF,
            .coilSensePin = NRF_SAADC_INPUT_DISABLED,
            .vbatSensePin = NRF_SAADC_INPUT_AIN2,
            .vledSensePin = NRF_SAADC_INPUT_DISABLED,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 21,
            .name = "Dev",
        },
        //D20Board
        {
            .boardResistorValueInKOhms = 20, // 20k Resistor
            .ledDataPin = 1,
            .ledClockPin = 4,
            .ledPowerPin = 0,
            .i2cDataPin = 12,
            .i2cClockPin = 14,
            .accInterruptPin = 15,
            .chargingStatePin = 0xFF,
            .coilSensePin = NRF_SAADC_INPUT_DISABLED,
            .vbatSensePin = NRF_SAADC_INPUT_AIN3,
            .vledSensePin = NRF_SAADC_INPUT_DISABLED,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 20,
            .name = "D20v1",
        },
        //D20BoardV5
        {
            .boardResistorValueInKOhms = 33, // 33k or 56k Resistor
            .ledDataPin = 0,
            .ledClockPin = 1,
            .ledPowerPin = 10,
            .i2cDataPin = 12,
            .i2cClockPin = 14,
            .accInterruptPin = 15,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN3,
            .vbatSensePin = NRF_SAADC_INPUT_AIN6,
            .vledSensePin = NRF_SAADC_INPUT_AIN2,
            .magnetPin = 9,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 20,
            .name = "D20v5",
        },
        // D6Board
        {
            .boardResistorValueInKOhms = 47, // 47k Resistor
            .ledDataPin = 1,
            .ledClockPin = 4,
            .ledPowerPin = 0,
            .i2cDataPin = 12,
            .i2cClockPin = 14,
            .accInterruptPin = 15,
            .chargingStatePin = 0xFF,
            .coilSensePin = NRF_SAADC_INPUT_DISABLED,
            .vbatSensePin = NRF_SAADC_INPUT_AIN3,
            .vledSensePin = NRF_SAADC_INPUT_DISABLED,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 6,
            .name = "D6v1",
        },
        //D20BoardV5
        {
            .boardResistorValueInKOhms = 56, // 33k or 56k Resistor
            .ledDataPin = 0,
            .ledClockPin = 1,
            .ledPowerPin = 10,
            .i2cDataPin = 12,
            .i2cClockPin = 14,
            .accInterruptPin = 15,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN3,
            .vbatSensePin = NRF_SAADC_INPUT_AIN6,
            .vledSensePin = NRF_SAADC_INPUT_AIN2,
            .magnetPin = 9,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 20,
            .name = "D20v5",
        },
        //D20BoardV8
        {
            .boardResistorValueInKOhms = 68, // 68k Resistor
            .ledDataPin = 1,
            .ledClockPin = 0,
            .ledPowerPin = 9,
            .i2cDataPin = 12,
            .i2cClockPin = 14,
            .accInterruptPin = 15,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN3,
            .vbatSensePin = NRF_SAADC_INPUT_AIN6,
            .vledSensePin = NRF_SAADC_INPUT_AIN2,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::LID2DE12,
            .ledModel = LEDModel::APA102,
            .ledCount = 20,
            .name = "D20v8",
        },
        // //D20BoardV9Alt1
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
        // //D20BoardV9Alt2
        // {
        //     .boardResistorValueInKOhms = 71, // 71.5k Resistor
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
        // //D20BoardV9Alt3
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
        //D20BoardV9Alt4
        {
            .boardResistorValueInKOhms = 79, // 78.7k Resistor
            .ledDataPin = 1,
            .ledClockPin = 0xFF,
            .ledPowerPin = 9,
            .i2cDataPin = 14,
            .i2cClockPin = 12,
            .accInterruptPin = 15,
            .chargingStatePin = 6,
            .coilSensePin = NRF_SAADC_INPUT_AIN3,
            .vbatSensePin = NRF_SAADC_INPUT_AIN6,
            .vledSensePin = NRF_SAADC_INPUT_AIN2,
            .magnetPin = 0xFF,
            .accModel = AccelerometerModel::MXC4005XC,
            .ledModel = LEDModel::NEOPIXEL_GRB,
            .ledCount = 20,
            .name = "D20v9.4",
        },
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
        // D20v5 board uses 33k over 100k voltage divider, or 56k over 100k (because I ran out of 33k 0402 resistors)
        // D20v3 board uses 20k over 100k voltage divider
        // i.e. the voltage read should be 3.1V * 20k / 120k = 0.55V
        // The D6v2 board uses 47k over 100k, i.e. 1.05V
        // The D20v2 board should read 0 (unconnected)
        // So we can allow a decent
        //const float vdd = DriversNRF::A2D::readVDD();
        const float vdd = DriversNRF::A2D::readVDD() + 0.1f;

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
