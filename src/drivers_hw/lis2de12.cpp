/******************************************************************************

Modified by Jean Simonet, Systemic Games

******************************************************************************/

#include "lis2de12.h"
#include "drivers_nrf/i2c.h"
#include "nrf_log.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/scheduler.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "drivers_nrf/gpiote.h"
#include "core/float3.h"
#include "utils/Utils.h"
#include "core/delegate_array.h"

using namespace DriversNRF;
using namespace Config;

namespace DriversHW
{
namespace LIS2DE12
{

    // LIS2DE12 Register Definitions
    enum Registers
    {
        STATUS_REG_AUX = 0x07,
        OUT_TEMP_L = 0x0C,
        OUT_TEMP_H = 0x0D,
        WHO_AM_I = 0x0F,
        CTRL_REG0 = 0x1E,
        TEMP_CFG_REG = 0x1F,
        CTRL_REG1 = 0x20,
        CTRL_REG2 = 0x21,
        CTRL_REG3 = 0x22,
        CTRL_REG4 = 0x23,
        CTRL_REG5 = 0x24,
        CTRL_REG6 = 0x25,
        REFERENCE = 0x26,
        STATUS_REG = 0x27,
        FIFO_READ_START = 0x28,
        OUT_X_H = 0x29,
        OUT_Y_H = 0x2B,
        OUT_Z_H = 0x2D,
        FIFO_CTRL_REG = 0x2E,
        FIFO_SRC_REG = 0x2F,
        INT1_CFG = 0x30,
        INT1_SRC = 0x31,
        INT1_THS = 0x32,
        INT1_DURATION = 0x33,
        INT2_CFG = 0x34,
        INT2_SRC = 0x35,
        INT2_THS = 0x36,
        INT2_DURATION = 0x37,
        CLICK_CFG = 0x38,
        CLICK_SRC = 0x39,
        CLICK_THS = 0x3A,
        TIME_LIMIT = 0x3B,
        TIME_LATENCY = 0x3C,
        TIME_WINDOW = 0x3D,
        ACT_THS = 0x3E,
        ACT_DUR = 0x3F,
    };

    // Defines the acceleration that can be measured (in Gs)
    enum FullScaleRange
    {
        FSR_2G = 0,
        FSR_4G = 1,
        FSR_8G = 2,
        FSR_16G = 3,
    };

    // Possible data rates
    enum DataRate
    {
        ODR_PWR_DWN = 0,
        ODR_1,
        ODR_10,
        ODR_25,
        ODR_50,
        ODR_100,
        ODR_200,
        ODR_400,
        ODR_1620,
        ODR_5376,
    };

    const uint8_t devAddress = 0x18;
    const uint16_t wakeUpThreshold = 32;
    const uint8_t wakeUpCount = 1;

    // Little macros to compute accelerations properly
    #define RANGE (FSR_4G)
    #define SCALE ((float)(1 << (RANGE + 1)))
    #define DATA_RATE (ODR_10)

    #define MAX_CLIENTS 2
	DelegateArray<LIS2DE12ClientMethod, MAX_CLIENTS> clients;

    void ApplySettings();
	void standby();
	void active();
    void reset();

	/// <summary>
	///	This function initializes the LIS2DE12. It sets up the scale (either 2, 4,
	///	or 8g), output data rate, portrait/landscape detection and tap detection.
	///	It also checks the WHO_AM_I register to make sure we can communicate with
	///	the sensor. Returns a 0 if communication failed, 1 if successful.
	/// </summary>
	/// <param name="fsr"></param>
	/// <param name="odr"></param>
	/// <returns></returns>
	void init()	{
		uint8_t c = I2C::readRegister(devAddress, WHO_AM_I);  // Read WHO_AM_I register

		if (c != 0x33) // WHO_AM_I should always be 0x33 on LIS2DE12
		{
			NRF_LOG_ERROR("LIS2DE12 - Bad WHOAMI - received 0x%02x, should be 0x33", c);
			return;
		}

        // Initialize settings
        ApplySettings();

        // lowPower();
        // reset();

		// Make sure our interrupts are cleared to begin with!
        disableInterrupt();
        clearInterrupt();
        enableDataInterrupt();

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
	/// </summary>
	void read(Core::float3* outAccel) {
        // Read accelerometer data

		int16_t x = I2C::readRegister(devAddress, OUT_X_H);
        if (x & 0x80) x |= 0xFF00;
		int16_t y = I2C::readRegister(devAddress, OUT_Y_H);
        if (y & 0x80) y |= 0xFF00;
		int16_t z = I2C::readRegister(devAddress, OUT_Z_H);
        if (z & 0x80) z |= 0xFF00;

		outAccel->x = (float)x * SCALE / (float)(1 << 7);
		outAccel->y = (float)y * SCALE / (float)(1 << 7);
		outAccel->z = (float)z * SCALE / (float)(1 << 7);
	}

    /// <summary>
    /// Turns the accelerometer on with current settings
    /// </summary>
    void ApplySettings() {
        standby();
        {
            // Scale
            I2C::writeRegister(devAddress, CTRL_REG4, (RANGE << 4));

            // Data Rate
            I2C::writeRegister(devAddress, CTRL_REG1, (DATA_RATE << 4) | 0b00000111);
        }
        active();
    }

	/// <summary>
	/// ENABLE INTERRUPT ON TRANSIENT MOTION DETECTION
	/// This function sets up the LIS2DE12 to trigger an interrupt on pin 1
	/// when it detects any motion (lowest detectable threshold).
	/// </summary>
	void enableInterrupt() {
        standby();
        {
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
        }
        active();
	}

    /// <summary>
    /// Interrupt handler when data is ready
    /// </summary>
	void dataInterruptHandler(uint32_t pin, nrf_gpiote_polarity_t action) {

        //I2C::readRegister(devAddress, STATUS_REG);

        // Trigger the callbacks
        Scheduler::push(nullptr, sizeof(Core::float3), [](void* accCopyPtr, uint16_t event_size) {
            Core::float3 accCopy;
            read(&accCopy);

            // // Cast the param to the right type
            // Core::float3* accCopy = (Core::float3*)(accCopyPtr);

            // NRF_LOG_INFO("x: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(accCopy.x));
            // NRF_LOG_INFO("y: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(accCopy.y));
            // NRF_LOG_INFO("z: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(accCopy.z));

            for (int i = 0; i < clients.Count(); ++i) {
                clients[i].handler(clients[i].token, accCopy);
            }
        });
        clearInterrupt();
    }

	/// <summary>
	/// Enable Data ready interrupt
	/// </summary>
	void enableDataInterrupt() {

		// Set interrupt pin
		GPIOTE::enableInterrupt(
			BoardManager::getBoard()->accInterruptPin,
			NRF_GPIO_PIN_NOPULL,
			NRF_GPIOTE_POLARITY_LOTOHI,
			dataInterruptHandler);

		// Enable interrupt on data update
		I2C::writeRegister(devAddress, CTRL_REG3, 0b00010000);
	}

	/// <summary>
	/// DISABLE TRANSIENT INTERRUPT
	/// </summary>
	void disableInterrupt() {
        standby();
        {
            // Disable interrupt on xyz axes
            I2C::writeRegister(devAddress, CTRL_REG3, 0b00000000);
        }
        active();
	}

	/// <summary>
	/// CLEARS TRANSIENT INTERRUPT
	/// This function will 'aknowledge' the transient interrupt from the device
	/// </summary>
	void clearInterrupt() {
		I2C::readRegister(devAddress, INT1_SRC);
	}

	/// <summary>
	/// SET STANDBY MODE
	///	Sets the LIS2DE12 to standby mode. It must be in standby to change most register settings
	/// </summary>
	void standby() {
		uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
		I2C::writeRegister(devAddress, CTRL_REG1, c & ~(0x08)); //Clear the active bit to go into standby
	}

	/// <summary>
	/// SET ACTIVE MODE
	///	Sets the LIS2DE12 to active mode. Needs to be in this mode to output data
	/// </summary>
	void active() {
		uint8_t c = I2C::readRegister(devAddress, CTRL_REG1);
		I2C::writeRegister(devAddress, CTRL_REG1, c | 0x08); //Set the active bit to begin detection
	}

	void lowPower() {
		I2C::writeRegister(devAddress, CTRL_REG1, 0b00001000);
	}

    void reset() {
        I2C::writeRegister(devAddress, CTRL_REG5, 0b10000000);
    }

	/// <summary>
	/// CHECK IF NEW DATA IS AVAILABLE
	///	This function checks the status of the LIS2DE12 to see if new data is availble.
	///	returns 0 if no new data is present, or a 1 if new data is available.
	/// </summary>
	uint8_t available()
	{
		return (I2C::readRegister(devAddress, FIFO_SRC_REG) & 0x1F);
	}

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hook(LIS2DE12ClientMethod method, void* parameter)
	{
		if (!clients.Register(parameter, method))
		{
			NRF_LOG_ERROR("Too many LIS2DE12 hooks registered.");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHook(LIS2DE12ClientMethod method)
	{
		clients.UnregisterWithHandler(method);
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHookWithParam(void* param)
	{
		clients.UnregisterWithToken(param);
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

