/******************************************************************************

Modified by Jean Simonet, Systemic Games

******************************************************************************/

#include "lis2dw12.h"
#include "drivers_nrf/i2c.h"
#include "nrf_log.h"
#include "../drivers_nrf/log.h"
#include "../drivers_nrf/power_manager.h"
#include "../drivers_nrf/timers.h"
#include "nrf_gpio.h"
#include "../config/board_config.h"
#include "../drivers_nrf/gpiote.h"
#include "../core/float3.h"
#include "../utils/Utils.h"

using namespace DriversNRF;
using namespace Config;

namespace DriversHW
{
namespace LIS2DW12
{

    // LIS2DE12 Register Definitions
    enum Registers
    {
        OUT_T_L = 0x0D,
        OUT_T_H = 0x0E,
        WHO_AM_I = 0x0F,
        CTRL1 = 0x20,
        CTRL2 = 0x21,
        CTRL3 = 0x22,
        CTRL4_INT1_PAD_CTRL = 0x23,
        CTRL5_INT2_PAD_CTRL = 0x24,
        CTRL6 = 0x25,
        OUT_T = 0x26,
        STATUS = 0x27,
        OUT_X_L = 0x28,
        OUT_X_H = 0x29,
        OUT_Y_L = 0x2A,
        OUT_Y_H = 0x2B,
        OUT_Z_L = 0x2C,
        OUT_Z_H = 0x2D,
        FIFO_CTRL = 0x2E,
        FIFO_SAMPLES = 0x2F,
        TAP_THS_X = 0x30,
        TAP_THS_Y = 0x31,
        TAP_THS_Z = 0x32,
        INT_DUR = 0x33,
        WAKE_UP_THS = 0x34,
        WAKE_UP_DUR = 0x35,
        FREE_FALL = 0x36,
        STATUS_DUP = 0x37,
        WAKE_UP_SRC = 0x38,
        TAP_SRC = 0x39,
        SIXD_SRC = 0x3A,
        ALL_INT_SRC = 0x3B,
        X_OFS_USR = 0x3C,
        Y_OFS_USR = 0x3D,
        Z_OFS_USR = 0x3E,
        CTRL_REG7 = 0x3F,
    };

    enum Scale
    {
        SCALE_2G = 0,
        SCALE_4G,
        SCALE_8G,
        SCALE_16G
    };

    enum DataRate
    {
        ODR_PWR_DWN = 0,
        ODR_1,
        ODR_12_5,
        ODR_25,
        ODR_50,
        ODR_100,
        ODR_200,
        ODR_400,
        ODR_800,
        ODR_1600,
    }; // possible data rates

    const uint8_t devAddress = 0x18;
    const Scale fsr = SCALE_4G;
    const float scaleMult = 4.0f;
    const DataRate dataRate = ODR_200;
    const uint16_t wakeUpThreshold = 32;
    const uint8_t wakeUpCount = 1;

    void ApplySettings();
	void standby();
	void active();

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
		uint8_t c = I2C::readRegister(devAddress, WHO_AM_I);  // Read WHO_AM_I register

		if (c != 0x33 && c != 0x44) // WHO_AM_I should always be 0x33 on LIS2DE12
		{
			NRF_LOG_ERROR("LIS2DE12 - Bad WHOAMI - received 0x%02x, should be 0x33 or 0x44", c);
			return;
		}

        // Initialize settings
        ApplySettings();

		// Make sure our interrupts are cleared to begin with!
		disableInterrupt();
		clearInterrupt();

		#if DICE_SELFTEST && LIS2DE12_SELFTEST
		selfTest();
		#endif
		#if DICE_SELFTEST && LIS2DE12_SELFTEST_INT
		selfTestInterrupt();
		#endif
		NRF_LOG_INFO("LIS2DE12 Initialized");
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
	void read(Core::float3* outAccel)
	{
		uint8_t x = I2C::readRegister(devAddress, OUT_X_H);
		uint8_t y = I2C::readRegister(devAddress, OUT_Y_H);
		uint8_t z = I2C::readRegister(devAddress, OUT_Z_H);
        NRF_LOG_INFO("xyz: %d, %d, %d", x, y, z);
		uint16_t xs = Utils::twosComplement(x);
		uint16_t ys = Utils::twosComplement(y);
		uint16_t zs = Utils::twosComplement(z);

		outAccel->x = (float)xs / (float)(1 << 7) * scaleMult;
		outAccel->y = (float)ys / (float)(1 << 7) * scaleMult;
		outAccel->z = (float)zs / (float)(1 << 7) * scaleMult;
	}

    void ApplySettings() {
		standby();

        // Scale
		uint8_t cfg = I2C::readRegister(devAddress, CTRL_REG4);
		cfg &= 0b11001111; // Mask out scale bits
		cfg |= (fsr << 4);
		I2C::writeRegister(devAddress, CTRL_REG4, cfg);

        // Data Rate
		uint8_t ctrl = I2C::readRegister(devAddress, CTRL_REG1);
		ctrl &= 0x0F; // Mask out data rate bits
		ctrl |= (dataRate << 4);
		I2C::writeRegister(devAddress, CTRL_REG1, ctrl);

		active();
    }

	/// <summary>
	/// ENABLE INTERRUPT ON TRANSIENT MOTION DETECTION
	/// This function sets up the MMA8452Q to trigger an interrupt on pin 1
	/// when it detects any motion (lowest detectable threshold).
	/// </summary>
	void enableInterrupt()
	{
		standby();

		// Enable OR of acceleration interrupt on any axis
		I2C::writeRegister(devAddress, INT1_CFG, 0b00101010);

		// Setup the high-pass filter
		//writeRegister(CTRL_REG2, 0b00110001);
		I2C::writeRegister(devAddress, CTRL_REG2, 0b00000000);

		// Setup the threshold
		I2C::writeRegister(devAddress, INT1_THS, wakeUpThreshold);

		// Setup the duration to minimum
		I2C::writeRegister(devAddress, INT1_DURATION, wakeUpCount);

		// Enable interrupt on xyz axes
		I2C::writeRegister(devAddress, CTRL_REG3, 0b01000000);

		active();
	}

	/// <summary>
	/// CLEARS TRANSIENT INTERRUPT
	/// This function will 'aknowledge' the transient interrupt from the device
	/// </summary>
	void clearInterrupt()
	{
		I2C::readRegister(devAddress, INT1_SRC);
	}

	/// <summary>
	/// DISABLE TRANSIENT INTERRUPT
	/// </summary>
	void disableInterrupt()
	{
		standby();
		// Disable interrupt on xyz axes
		I2C::writeRegister(devAddress, CTRL_REG3, 0b00000000);
		active();
	}

	/// <summary>
	/// SET STANDBY MODE
	///	Sets the MMA8452 to standby mode. It must be in standby to change most register settings
	/// </summary>
	void standby()
	{
		uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
		I2C::writeRegister(devAddress, CTRL_REG1, c & ~(0x08)); //Clear the active bit to go into standby
	}

	/// <summary>
	/// SET ACTIVE MODE
	///	Sets the MMA8452 to active mode. Needs to be in this mode to output data
	/// </summary>
	void active()
	{
		uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
		I2C::writeRegister(devAddress, CTRL_REG1, c | 0x08); //Set the active bit to begin detection
	}

	void lowPower() {
		I2C::writeRegister(devAddress, CTRL_REG1, 0b00001000); //Set the active bit to begin detection
	}

	/// <summary>
	/// CHECK IF NEW DATA IS AVAILABLE
	///	This function checks the status of the MMA8452Q to see if new data is availble.
	///	returns 0 if no new data is present, or a 1 if new data is available.
	/// </summary>
	uint8_t available()
	{
		return (I2C::readRegister(devAddress, FIFO_SRC_REG) & 0x1F);
	}

	#if DICE_SELFTEST && LIS2DE12_SELFTEST
    APP_TIMER_DEF(readAccTimer);
    void readAcc(void* context) {
		read();
        NRF_LOG_INFO("x=%d, cx=" NRF_LOG_FLOAT_MARKER, x, NRF_LOG_FLOAT(cx));
        NRF_LOG_INFO("y=%d, cy=" NRF_LOG_FLOAT_MARKER, y, NRF_LOG_FLOAT(cy));
        NRF_LOG_INFO("z=%d, cz=" NRF_LOG_FLOAT_MARKER, z, NRF_LOG_FLOAT(cz));
    }

    void selfTest() {
        Timers::createTimer(&readAccTimer, APP_TIMER_MODE_REPEATED, readAcc);
        NRF_LOG_INFO("Reading Acc, press any key to abort");
        Log::process();

        Timers::startTimer(readAccTimer, 1000, nullptr);
        // while (!Log::hasKey()) {
        //     Log::process();
		// 	PowerManager::feed();
        //     PowerManager::update();
        // }
		// Log::getKey();
        // NRF_LOG_INFO("Stopping to read acc!");
        // Timers::stopTimer(readAccTimer);
        // Log::process();
    }
	#endif

	#if DICE_SELFTEST && LIS2DE12_SELFTEST_INT
	bool interruptTriggered = false;
	void accInterruptHandler(uint32_t pin, nrf_gpiote_polarity_t action) {
		// pin and action don't matter
		interruptTriggered = true;
	}

    void selfTestInterrupt() {
        NRF_LOG_INFO("Setting accelerator to trigger interrupt");

		// Set interrupt pin
		GPIOTE::enableInterrupt(
			BoardManager::getBoard()->accInterruptPin,
			NRF_GPIO_PIN_NOPULL,
			NRF_GPIOTE_POLARITY_LOTOHI,
			accInterruptHandler);

		enableTransientInterrupt();
        Log::process();
        while (!interruptTriggered) {
            Log::process();
			PowerManager::feed();
            PowerManager::update();
        }
        NRF_LOG_INFO("Interrupt triggered!");
        Log::process();
    }
	#endif

}
}

