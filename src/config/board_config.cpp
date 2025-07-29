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
    static const Board board = {
        .boardResistorValueInKOhms = 0, // The resistors are soldered the wrong way, so we measure almost 0 volts.
        .ledDataPin = 15,
        .ledPowerPin = 17,
        .ledReturnPin = 12,
        .i2cDataPin = 18,
        .i2cClockPin = 16,
        .accInterruptPin = 20,
        .chargingStatePin = 8,
        .coilSensePin = NRF_SAADC_INPUT_AIN3,
        .vbatSensePin = NRF_SAADC_INPUT_AIN1,
        .ntcSensePin = NRF_SAADC_INPUT_AIN2,
        .progPin = 11,
        .ledCount = 20,
        .debugLedIndex = 0,
        .model = PyreV1,
        .name = "Pyre-V1",
    };


    // // Array of possible circuit boards configs
    // // Note that the boards MUST be sorted in order of INCREASING resistor value
    // // for the init method to properly find the correct board config.
    // static const Board boards[] = {
    //     {
    //         .boardResistorValueInKOhms = 0, // The resistors are soldered the wrong way, so we measure almost 0 volts.
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 0xFF,
    //         .ledCount = 21,
    //         .debugLedIndex = 20,
    //         .model = PD6BoardV3,
    //         .name = "PD6v3-BadR9",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 50, // 50.0k Resistor, at VCC = 3.1V, this means 3.1V * 50k / 150k = 1.03V
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10  ,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 21,
    //         .debugLedIndex = 20,
    //         .model = D6BoardV9,
    //         .name = "D6v9",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 62, // 62.0k Resistor, at VCC = 3.1V, this means 3.1V * 62k / 162k = 1.18V
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10  ,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 6,
    //         .debugLedIndex = 3,
    //         .model = D6BoardV6,
    //         .name = "D6v6",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 80, // 80.0k Resistor, at VCC = 3.1V, this means 3.1V * 80k / 180k = 1.37V
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 21,
    //         .debugLedIndex = 20,
    //         .model = PD6BoardV5,
    //         .name = "PD6v5",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 100, // 100.0k Resistor, at VCC = 3.1V, this means 3.1V * 100k / 200k = 1.55V
    //         .ledDataPin = 6,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 15,
    //         .i2cClockPin = 14,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 1,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 20,
    //         .debugLedIndex = 14,
    //         .model = D20BoardV15,
    //         .name = "D20v15",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 120, // 120.0k Resistor, at VCC = 3.1V, this means 3.1V * 120k / 220k = 1.69V
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10  ,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 0xFF,
    //         .ledCount = 6,
    //         .debugLedIndex = 3,
    //         .model = D6BoardV4,
    //         .name = "D6v2",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 150, // 150.0k Resistor, at VCC = 3.1V, this means 3.1V * 150k / 250k = 1.86V
    //         .ledDataPin = 1,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 0xFF,
    //         .ledCount = 21,
    //         .debugLedIndex = 20,
    //         .model = PD6BoardV3,
    //         .name = "PD6v3",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 180, // 180.0k Resistor, at VCC = 3.1V, this means 3.1V * 180k / 280k = 1.99V
    //         .ledDataPin = 6,
    //         .ledPowerPin = 0,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 15,
    //         .i2cClockPin = 14,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 1,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 12,
    //         .debugLedIndex = 6,
    //         .model = D12BoardV2,
    //         .name = "D12v1",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 220, // 220.0k Resistor, at VCC = 3.1V, this means 3.1V * 220k / 320k = 2.13V
    //         .ledDataPin = 0,
    //         .ledPowerPin = 10,
    //         .ledReturnPin = 1,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 19,
    //         .debugLedIndex = 18,
    //         .model = D00BoardV1,
    //         .name = "D00v1",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 270, // 270.0k Resistor, at VCC = 3.1V, this means 3.1V * 270k / 370k = 2.26V
    //         .ledDataPin = 0,
    //         .ledPowerPin = 10,
    //         .ledReturnPin = 1,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 6,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN3,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN6,
    //         .progPin = 9,
    //         .ledCount = 10,
    //         .debugLedIndex = 9,
    //         .model = D10BoardV2,
    //         .name = "D10v1",
    //     },
    //     {
    //         .boardResistorValueInKOhms = 470,
    //         .ledDataPin = 9,
    //         .ledPowerPin = 6,
    //         .ledReturnPin = 10,
    //         .i2cDataPin = 14,
    //         .i2cClockPin = 15,
    //         .accInterruptPin = 12,
    //         .chargingStatePin = 0,
    //         .coilSensePin = NRF_SAADC_INPUT_AIN2,
    //         .vbatSensePin = NRF_SAADC_INPUT_AIN6,
    //         .ntcSensePin = NRF_SAADC_INPUT_AIN3,
    //         .progPin = 1,
    //         .ledCount = 8,
    //         .debugLedIndex = 4,
    //         .model = D8BoardV2,
    //         .name = "D8v1",
    //     },
    // };

    // The board we're currently using
    static const Board* currentBoard = &board;

    void init() {
        // // Sample adc board pin
        // setNTC_ID_VDD(true);

        // int32_t vboardTimes1000 = DriversNRF::A2D::readVBoardTimes1000();

        // // Now that we're done reading, we can turn off the drive pin
        // setNTC_ID_VDD(false);

        // // Do some computation to figure out which variant we're working with!
        // // D20v3 board uses 20k over 100k voltage divider
        // // i.e. the voltage read should be 3.1V * 20k / 120k = 0.55V
        // // The D6v2 board uses 47k over 100k, i.e. 1.05V
        // // The D20v2 board should read 0 (unconnected)
        // // So we can allow a decent
        // const int32_t vddTimes1000 = DriversNRF::A2D::readVDDTimes1000();

        // // Compute board voltages
        // const int boardCount = sizeof(boards) / sizeof(boards[0]);
        // int32_t boardVoltagesTimes1000[boardCount];
        // for (int i = 0; i < boardCount; ++i) {
        //     boardVoltagesTimes1000[i] = (vddTimes1000 * boards[i].boardResistorValueInKOhms * 1000) / (BOARD_DETECT_RESISTOR + boards[i].boardResistorValueInKOhms * 1000);
        //     NRF_LOG_DEBUG("%s: voltage: %d.%03d", boards[i].name, boardVoltagesTimes1000[i] / 1000, boardVoltagesTimes1000[i] % 1000);
        // }
        // int32_t midpointVoltagesTimes1000[boardCount-1];
        // for (int i = 0; i < boardCount-1; ++i) {
        //     midpointVoltagesTimes1000[i] = (boardVoltagesTimes1000[i] + boardVoltagesTimes1000[i+1]) / 2;
        // }

        // // Find the first midpoint voltage that is above the measured voltage
        // int boardIndex = 0;
        // for (; boardIndex < boardCount-1; ++boardIndex) {
        //     if (midpointVoltagesTimes1000[boardIndex] > vboardTimes1000)
        //     break;
        // }
        // currentBoard = &(boards[boardIndex]);
        // NRF_LOG_INFO("Board is %s, boardId V: %d.%03d", currentBoard->name, vboardTimes1000 / 1000, vboardTimes1000 % 1000);
        NRF_LOG_INFO("Board is %s", currentBoard->name);
    }

    const Board* getBoard() {
        return currentBoard;
    }

    void setNTC_ID_VDD(bool set) {
        // if (set) {
        //     nrf_gpio_cfg_output(BOARD_DETECT_DRIVE_PIN);
        //     nrf_gpio_pin_set(BOARD_DETECT_DRIVE_PIN);
        // } else {
        //     nrf_gpio_cfg_default(BOARD_DETECT_DRIVE_PIN);
        // }
    }

}
}
 