#include "kxtj3-1057.h"
#include "drivers_nrf/i2c.h"
#include "nrf_log.h"
#include "../core/float3.h"
#include "../utils/Utils.h"

using namespace DriversNRF;
using namespace Config;

namespace DriversHW
{
namespace KXTJ3
{

    enum Registers
    {
        WHO_AM_I = 0x0F,
        DCST_RESP = 0x0C,	// used to verify proper integrated circuit functionality.It always has a byte value of 0x55

        OUT_X_L = 0x06,
        OUT_X_H = 0x07,
        OUT_Y_L = 0x08,
        OUT_Y_H = 0x09,
        OUT_Z_L = 0x0A,
        OUT_Z_H = 0x0B,

        STATUS = 0x18,
        INT_SOURCE1 = 0x16,
        INT_SOURCE2 = 0x17,

        INT_REL = 0x1A,
        CTRL_REG1 = 0x1B,
        CTRL_REG2 = 0x1D,

        INT_CTRL_REG1 = 0x1E,
        INT_CTRL_REG2 = 0x1F,

        DATA_CTRL_REG = 0x21,
        WAKEUP_COUNTER = 0x29,
        NA_COUNTER = 0x2A,

        WAKEUP_THRD_H = 0x6A,
        WAKEUP_THRD_L = 0x6B,
    };

    enum Scale
    {
        SCALE_2G  = 0,
        SCALE_4G  = 2,
        SCALE_8G  = 4,
        SCALE_16G = 1,
    };

    enum DataRate
    {
        ODR_0_781 = 8,
        ODR_1_563 = 9,
        ODR_3_125 = 10,
        ODR_6_25  = 11,
        ODR_12_5  = 0,
        ODR_25    = 1,
        ODR_50    = 2,
        ODR_100   = 3,
        ODR_200   = 4,
        ODR_400   = 5,
        ODR_800   = 6,
        ODR_1600  = 7,
    };

    const uint8_t devAddress = 0x0E;
    const Scale fsr = SCALE_4G;
    const float scaleMult = 4.0f;
    const DataRate dataRate = ODR_200;
    const uint16_t wakeUpThreshold = 32;
    const uint8_t wakeUpCount = 1;

    void ApplySettings();
	void standby();
	void active();

	void init()
	{
		uint8_t c = I2C::readRegister(devAddress, WHO_AM_I);  // Read WHO_AM_I register

		if (c != 0x35) // WHO_AM_I should always be 0x35 on KXTJ3
		{
			NRF_LOG_ERROR("KXTJ3 - Bad WHOAMI - received 0x%02x, should be 0x35", c);
			return;
		}

        // Initialize settings
        ApplySettings();

		// Make sure our interrupts are cleared to begin with!
		disableInterrupt();
		clearInterrupt();

		NRF_LOG_INFO("KXTJ3 Initialized");
	}

    void read(Core::float3* outAccel) {

        uint16_t cx, cy, cz;
        cx = Utils::twosComplement16(I2C::readRegisterInt16(devAddress, OUT_X_L));
        cy = Utils::twosComplement16(I2C::readRegisterInt16(devAddress, OUT_Y_L));
        cz = Utils::twosComplement16(I2C::readRegisterInt16(devAddress, OUT_Z_L));
        outAccel->x = (float)cx / (float)(1 << 11) * scaleMult;
        outAccel->y = (float)cy / (float)(1 << 11) * scaleMult;
        outAccel->z = (float)cz / (float)(1 << 11) * scaleMult;
    }

    void standby()
    {
        uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
        I2C::writeRegister(devAddress, CTRL_REG1, c & ~(0x08)); //Clear the active bit to go into standby
    }

    void active()
    {
        uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
        I2C::writeRegister(devAddress, CTRL_REG1, c | 0x08); //Set the active bit to begin detection
    }

    void ApplySettings() {
		standby();

        // Scale
		uint8_t cfg = I2C::readRegister(devAddress, CTRL_REG1);
		cfg &= 0b11100011; // Mask out scale bits
		cfg |= (fsr << 2);
		I2C::writeRegister(devAddress, CTRL_REG1, cfg);

        // Data Rate
		uint8_t ctrl = I2C::readRegister(devAddress, DATA_CTRL_REG);
		ctrl &= 0b11111000; // Mask out data rate bits
		ctrl |= dataRate;
		I2C::writeRegister(devAddress, DATA_CTRL_REG, ctrl);

		active();
    }

	void enableInterrupt()
	{
		standby();

        // Enable interrupt, active High, latched
		I2C::writeRegister(devAddress, INT_CTRL_REG1, 0b00110010);

    	// WUFE – enables the Wake-Up (motion detect) function.
    	uint8_t _reg1 = I2C::readRegister(devAddress, CTRL_REG1);
    	_reg1 |= (0x01 << 1);
		I2C::writeRegister(devAddress, CTRL_REG1, _reg1);

        // enable interrupt on all axis any direction - Latched
    	I2C::writeRegister(devAddress, INT_CTRL_REG2, 0b00111111);

    	// Set WAKE-UP (motion detect) Threshold
    	I2C::writeRegister(devAddress, WAKEUP_THRD_H, (uint8_t)(wakeUpThreshold >> 4));
    	I2C::writeRegister(devAddress, WAKEUP_THRD_L, (uint8_t)(wakeUpThreshold << 4));

	    // WAKEUP_COUNTER -> Sets the time motion must be present before a wake-up interrupt is set
	    // WAKEUP_COUNTER (counts) = Wake-Up Delay Time (sec) x Wake-Up Function ODR(Hz)
	    I2C::writeRegister(devAddress, WAKEUP_COUNTER, wakeUpCount);

		active();
	}

	void clearInterrupt()
	{
		I2C::readRegister(devAddress, INT_REL);
	}

	void disableInterrupt()
	{
		standby();
		// Disable interrupt on xyz axes
		I2C::writeRegister(devAddress, INT_CTRL_REG1, 0b00010000);
		active();
	}

}
}
