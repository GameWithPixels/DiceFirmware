#include "coil.h"
#include "nrf_assert.h"
#include "nrf_log.h"
#include "board_config.h"
#include "drivers_nrf/gpiote.h"
#include "drivers_nrf/a2d.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/scheduler.h"
#include "drivers_hw/battery.h"

using namespace DriversNRF;
using namespace Config;

// Coil voltage is 0.0V or 5.0V nominal (can't go negative)
#define VCOIL_LOW_THRESHOLD (-1000) // mV
#define VCOIL_HIGH_THRESHOLD 7000 // mV

#define COIL_MEASURE_INTERVAL 100   // ms
#define COIL_MEASUREMENT_COUNT 18  // count
#define COIL_UPDATE_MS_FAST 1000    // ms
//#define COIL_UPDATE_MS_SLOW 5000    // ms

#define ChargerOn 4800 // >
#define ChargerOff 3000 // <

#define ChargerPingMin_FD 3000 // <
#define ChargerPingMax_FD 4800 // >
#define ChargerPingMin 1500 // <
#define ChargerPingMax 3000 // >


namespace DriversHW
{
namespace Coil
{
    const int32_t vCoilMultTimes1000 = 2000; // Voltage divider 4M over 4M
    APP_TIMER_DEF(coilTimer);
    int32_t vCoilTimes1000;
    int32_t vcoilMinTimes1000;
    int32_t vcoilMaxTimes1000;

    int32_t vcoilMeasurementAccumulator;
    int32_t vcoilMeasurementCount;
    int32_t vcoilMeasurementMin;
    int32_t vcoilMeasurementMax;

    CoilState coilState;

    int32_t checkVCoilTimes1000();
    void update(void* param);

    bool init() {
        vCoilTimes1000 = checkVCoilTimes1000();
        vcoilMinTimes1000 = vCoilTimes1000;
        vcoilMaxTimes1000 = vCoilTimes1000;
        coilState = CoilState_Off;

        // Check that the measured voltages are in a valid range
        bool success = vCoilTimes1000 > VCOIL_LOW_THRESHOLD && vCoilTimes1000 < VCOIL_HIGH_THRESHOLD;
        if (!success) {
            NRF_LOG_ERROR("Coil Voltage invalid, VCoil: %d.%03d", vCoilTimes1000 / 1000, vCoilTimes1000 % 1000);
        }

        vcoilMeasurementCount = 0;
        vcoilMeasurementMin = 10000;
        vcoilMeasurementMax = 0;

        Timers::createTimer(&coilTimer, APP_TIMER_MODE_SINGLE_SHOT, update);
        Timers::startTimer(coilTimer, COIL_UPDATE_MS_FAST);

        NRF_LOG_INFO("Coil init");
        NRF_LOG_INFO("  Voltage: %d.%03d", vCoilTimes1000 / 1000, vCoilTimes1000 % 1000);
        return success;
    }

    int32_t getVCoilTimes1000() {
        return vCoilTimes1000;
    }

    int32_t getVCoilMinTimes1000() {
        return vcoilMinTimes1000;
    }
    
    int32_t getVCoilMaxTimes1000() {
        return vcoilMaxTimes1000;
    }

    int32_t checkVCoilTimes1000() {
        int32_t ret = A2D::read5VTimes1000() * vCoilMultTimes1000 / 1000;
        return ret;
    }

    void update(void* param) {
        // Start a measurement burst
        auto vcoil = checkVCoilTimes1000();
        vcoilMeasurementAccumulator += vcoil;

        vcoilMeasurementMin = MIN(vcoilMeasurementMin, vcoil);
        vcoilMeasurementMax = MAX(vcoilMeasurementMax, vcoil);

        vcoilMeasurementCount++;
        if (vcoilMeasurementCount == COIL_MEASUREMENT_COUNT) {
            // Average the measurements
            vCoilTimes1000 = vcoilMeasurementAccumulator / COIL_MEASUREMENT_COUNT;
            vcoilMinTimes1000 = vcoilMeasurementMin;
            vcoilMaxTimes1000 = vcoilMeasurementMax;

            // Reset counter/accumulator
            vcoilMeasurementCount = 0;
            vcoilMeasurementAccumulator = 0;
            vcoilMeasurementMin = 10000;
            vcoilMeasurementMax = 0;

            // Determine coil state
            if (vcoilMaxTimes1000 > ChargerOn && vcoilMinTimes1000 > ChargerOn) {
                coilState = CoilState_On;
            } else if (vcoilMaxTimes1000 < ChargerOff && vcoilMinTimes1000 < ChargerOff) {
                coilState = CoilState_Off;
            } else {
                if (Battery::getDisableCharging()) {
                    if (vcoilMaxTimes1000 > ChargerPingMax_FD && vcoilMinTimes1000 < ChargerPingMin_FD) {
                        coilState = CoilState_Pinging;
                    }
                    // Else keep current state
                } else {
                    if (vcoilMaxTimes1000 > ChargerPingMax && vcoilMinTimes1000 < ChargerPingMin) {
                        coilState = CoilState_Pinging;
                    }
                    // Else keep current state
                }
            }

            // Setup timer for next burst
            Timers::startTimer(coilTimer, COIL_UPDATE_MS_FAST);
        } else {
            // Just keep measuring
            Timers::startTimer(coilTimer, COIL_MEASURE_INTERVAL);
        }
    }

    CoilState getCoilState() {
        return coilState;
    }
}
}