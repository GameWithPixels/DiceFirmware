#include "ntc.h"
#include "nrf_assert.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "board_config.h"
#include "drivers_nrf/a2d.h"


using namespace DriversNRF;
using namespace Config;

#define NTC_LOOKUP_SIZE 11

namespace DriversHW::NTC
{
    struct VoltageAndTemperature
    {
        uint16_t voltageTimes1000;
        uint16_t temperatureTimes100;
    };

    // This lookup table defines our voltage to temperature curve
    // based on the NTC datasheet and voltage divider (100k resistor)
    static const VoltageAndTemperature lookup[NTC_LOOKUP_SIZE] =
    {
        {2350,  0000}, // 2.35V -> 0 C
        {2040,  1000},
        {1670,  2000},
        {1320,  3000},
        {1000,  4000},
        { 750,  5000},
        { 550,  6000},
        { 400,  7000},
        { 280,  8000},
        { 200,  9000},
        { 150, 10000}, // 0.15V -> 100 C
    };

    float lookupTemperature(float voltage);

    void init() {
        // Grab initial values from the battery driver
        float temp = getNTCTemperature();
        NRF_LOG_INFO("NTC Initialized, battery temperature = " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(temp));
    }


    float getNTCTemperature() {
        // Sample adc board pin

        // Turn VDD on
        BoardManager::setNTC_ID_VDD(true);

        // Workaround for early D20V15
        // Wait for voltage to rise
        nrf_delay_ms(10);

        // Read voltage divider
        float vntc = A2D::readVNTC();

        // Now that we're done reading, we can turn off the drive pin
        BoardManager::setNTC_ID_VDD(false);

        // Calculate temperature from voltage
        return lookupTemperature(vntc);
    }

    float lookupTemperature(float voltage)
    {
        // Convert voltage to integer so we can quickly compare it with the lookup table
        int voltageTimes1000 = (int)(voltage * 1000.0f);

		// Find the first voltage that is greater than the measured voltage
        // Because voltages are sorted, we know that we can then linearly interpolate the temperature
        // using the previous and next entries in the lookup table.
		int nextIndex = 0;
        while (nextIndex < NTC_LOOKUP_SIZE && lookup[nextIndex].voltageTimes1000 >= voltageTimes1000) {
            nextIndex++;
        }

		int temperatureTimes100 = 0;
		if (nextIndex == 0) {
			temperatureTimes100 = 0;
		} else if (nextIndex == NTC_LOOKUP_SIZE) {
			temperatureTimes100 = 10000;
		} else {
			// Grab the prev and next keyframes
            auto next = lookup[nextIndex];
            auto prev = lookup[nextIndex - 1];


			// Compute the interpolation parameter
    		int percentTimes1000 = ((int)prev.voltageTimes1000 - (int)voltageTimes1000) * 1000 / ((int)prev.voltageTimes1000 - (int)next.voltageTimes1000);
            temperatureTimes100 = ((int)prev.temperatureTimes100 * (1000 - percentTimes1000) + (int)next.temperatureTimes100 * percentTimes1000) / 1000;
		}

		return (float)(temperatureTimes100) / 100.0f;
    }
}

