/******************************************************************************

Modified by Jean Simonet, Systemic Games

******************************************************************************/

#include "mxc4005xc.h"
#include "drivers_nrf/i2c.h"
#include "nrf_log.h"
#include "../drivers_nrf/log.h"
#include "../drivers_nrf/power_manager.h"
#include "../drivers_nrf/timers.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "../config/board_config.h"
#include "../drivers_nrf/gpiote.h"
#include "../core/float3.h"
#include "../utils/Utils.h"

using namespace DriversNRF;
using namespace Config;

namespace DriversHW
{
namespace MXC4005XC
{

    // LIS2DE12 Register Definitions
    enum Registers
    {
        INT_SRC0 = 0x00,
        INT_CLR0 = 0x00,
        INT_SRC1 = 0x01,
        INT_CLR1 = 0x01,
        STATUS = 0x02,
        XOUT_H = 0x03,
        XOUT_L = 0x04,
        YOUT_H = 0x05,
        YOUT_L = 0x06,
        ZOUT_H = 0x07,
        ZOUT_L = 0x08,
        TOUT = 0x09,
        INT_MASK0 = 0x0A,
        INT_MASK1 = 0x0B,
        DETECTION = 0x0C,
        CONTROL = 0x0D,
        DEVICE_ID = 0x0E,
        WHO_AM_I = 0x0F,
    };

    enum Scale
    {
        SCALE_2G = 0,
        SCALE_4G,
        SCALE_8G,
    };

    const uint8_t devAddress = 0x15;
    const Scale fsr = SCALE_4G;
    const float scaleMult = 4.0f;

    void ApplySettings();

	/// <summary>
	///	This function initializes the LIS2DE12. It sets up the scale (either 2, 4,
	///	or 8g), output data rate, portrait/landscape detection and tap detection.
	///	It also checks the WHO_AM_I register to make sure we can communicate with
	///	the sensor. Returns a 0 if communication failed, 1 if successful.
	/// </summary>
	/// <param name="fsr"></param>
	/// <param name="odr"></param>
	/// <returns></returns>
	void init()
	{
		uint8_t c = I2C::readRegister(devAddress, DEVICE_ID);  // Read WHO_AM_I register

		if (c != 0x3)
		{
			NRF_LOG_ERROR("MXC4005XC - Bad DEVICE_ID - received 0x%02x, should be 0x03", c);
			return;
		}

        // Initialize settings
        ApplySettings();

		// Make sure our interrupts are cleared to begin with!
		disableInterrupt();
		clearInterrupt();

		NRF_LOG_INFO("MXC4005XC Initialized");
	}

	/// <summary>
	/// READ ACCELERATION DATA
	///  This function will read the acceleration values from the MMA8452Q. After
	///	reading, it will update two triplets of variables:
	///		* int's x, y, and z will store the signed 12-bit values read out
	///		  of the acceleromter.
	///		* floats cx, cy, and cz will store the calculated acceleration from
	///		  those 12-bit values. These variables are in units of g's.
	/// </summary>
    void read(Core::float3* outAccel) {

        uint16_t cx, cy, cz;
        cx = I2C::readRegisterInt16(devAddress, XOUT_H);
        cy = I2C::readRegisterInt16(devAddress, YOUT_H);
        cz = I2C::readRegisterInt16(devAddress, ZOUT_H);

        cx = ((cx & 0x00FF) << 4) | ((cx & 0xFF00) >> 8);
        cy = ((cy & 0x00FF) << 4) | ((cy & 0xFF00) >> 8);
        cz = ((cz & 0x00FF) << 4) | ((cz & 0xFF00) >> 8);

        int sx = Utils::twosComplement12(cx);
        int sy = Utils::twosComplement12(cy);
        int sz = Utils::twosComplement12(cz);

        // Convert to float
        outAccel->x = (float)sx * scaleMult / (float)(1 << 11);
        outAccel->y = (float)sy * scaleMult / (float)(1 << 11);
        outAccel->z = (float)sz * scaleMult / (float)(1 << 11);
    }

    void ApplySettings() {

        // Scale
		uint8_t cfg = I2C::readRegister(devAddress, CONTROL);
		cfg &= 0b10011111; // Mask out scale bits
		cfg |= (fsr << 5);
		I2C::writeRegister(devAddress, CONTROL, cfg);
    }

	/// <summary>
	/// ENABLE INTERRUPT ON TRANSIENT MOTION DETECTION
	/// This function sets up the MMA8452Q to trigger an interrupt on pin 1
	/// when it detects any motion (lowest detectable threshold).
	/// </summary>
	void enableInterrupt()
	{
		// Detect orientation or shake, lowest thresholds
		I2C::writeRegister(devAddress, DETECTION, 0b10000000);

        // Set interrupt mask, enable all shake or orientation
		I2C::writeRegister(devAddress, INT_MASK0, 0b11001111);

		// Don't trigger on data ready
		I2C::writeRegister(devAddress, INT_MASK1, 0x00);
	}

	/// <summary>
	/// CLEARS TRANSIENT INTERRUPT
	/// This function will 'aknowledge' the transient interrupt from the device
	/// </summary>
	void clearInterrupt()
	{
        uint8_t src0 = I2C::readRegister(devAddress, INT_SRC0);
		I2C::writeRegister(devAddress, INT_CLR0, src0);

        uint8_t src1 = I2C::readRegister(devAddress, INT_SRC1);
		I2C::writeRegister(devAddress, INT_CLR1, src1);
	}

	/// <summary>
	/// DISABLE TRANSIENT INTERRUPT
	/// </summary>
	void disableInterrupt()
	{
        // Clear interrupt mask
		I2C::writeRegister(devAddress, INT_MASK0, 0x00);
	}

}
}

