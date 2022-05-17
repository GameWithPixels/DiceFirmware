/******************************************************************************

Modified by Jean Simonet, Systemic Games

******************************************************************************/

#include "accel_chip.h"
#include "drivers_nrf/i2c.h"
#include "nrf_log.h"
#include "drivers_nrf/log.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/scheduler.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "drivers_nrf/gpiote.h"
#include "core/float3.h"
#include "utils/Utils.h"
#include "core/delegate_array.h"

using namespace DriversNRF;
using namespace Config;

namespace DriversHW
{
namespace AccelChip
{
    // MXC4005XC Register Definitions
    enum Registers : uint8_t
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
        PASSWORD = 0x0E,
        WHO_AM_I = 0x0F,
        AOZL = 0x23,
        AOZH = 0x24,
        AGZL = 0x25,
        AGZH = 0x26,
    };

    // Useful values to write into registers
    enum Command : uint8_t
    {
        CMD_POWER_DOWN	= 0x01,
        CMD_PASSWORD	= 0x93,
        CMD_RESET       = 0x10,
    };

    // Defines the acceleration that can be measured (in Gs)
    enum FullScaleRange
    {
        FSR_2G = 0,
        FSR_4G = 1,
        FSR_8G = 2,
    };

    const uint8_t DEVICE_ID_1 = 0x02;
    const uint8_t DEVICE_ID_2 = 0x03;
    const float T_ZERO = 25.0f;
    const float T_SENSITIVITY = 0.586f;

    const uint8_t devAddress = 0x15;

    // Little macros to compute accelerations properly
    #define RANGE (FSR_8G)
    #define FSR_COMMAND (RANGE << 5)
    #define SENSITIVITY (1.0f / (float)(1024 >> RANGE))

    #define MAX_CLIENTS 2
	DelegateArray<AccelClientMethod, MAX_CLIENTS> clients;

    #define READING_COUNTS 10
    int16_t zoffset = 0;

    // Small struct that stores a running sum of measurements (to compute an averaged value)
    struct AccelerometerAccumulator
    {
        int16_t xSum;
        int16_t ySum;
        int16_t zSum;
        int16_t tSum;
        uint16_t count;
        void clear() {
            xSum = 0;
            ySum = 0;
            zSum = 0;
            tSum = 0;
            count = 0;
        }
        bool add(int16_t x, int16_t y, int16_t z, int16_t t) {
            xSum += x;
            ySum += y;
            zSum += z;
            tSum += t;
            count++;
            return count == READING_COUNTS;
        }
    };
    AccelerometerAccumulator sums;

    void reset();
    int AccAozHandle(float tmpr,float zoff, int z_dir);
    void SetAozPara(float offset);
    void clearDataInterrupt();

	/// <summary>
	///	This function initializes the MXC4005XC. It sets up the scale (either 2, 4 or 8g)
	/// </summary>
	void init()
	{
        // Cache the Z offset from the saved settings
        zoffset = SettingsManager::getSettings()->MXC4005XCZOffset;

        // Zero the running sums
        sums.clear();

        // Start by reseting the chip
        reset();

		uint8_t devId = I2C::readRegister(devAddress, DEVICE_ID);  // Read ID
		if (devId != DEVICE_ID_1 && devId != DEVICE_ID_2)
		{
			NRF_LOG_ERROR("MXC4005XC - Bad DEVICE_ID - received 0x%02x", devId);
			return;
		}

        // Check who_am_i
		uint8_t wai = I2C::readRegister(devAddress, WHO_AM_I);  // Read WHO_AM_I register

        /* Set the acceleration range */
		I2C::writeRegister(devAddress, CONTROL, FSR_COMMAND);

        nrf_delay_ms(300);

        // -666
        // -1175
        // Mid: -920

        //SetAozPara(0);
        clearInterrupt();
        disableInterrupt();
        enableDataInterrupt();

        NRF_LOG_INFO("MXC4005XC Initialized, WAI: 0x%02x", wai);
	}

	/// <summary>
	/// READ ACCELERATION DATA
	/// </summary>
	void read(Core::float3* outAccel) {
        // Read accelerometer data
        uint8_t accBuffer[6];
        I2C::readRegisters(devAddress, XOUT_H, accBuffer, 6);

        // Convert acc
        int16_t cx = (((int16_t)accBuffer[0] << 8) | accBuffer[1]) >> 4;
        if (cx & 0x0800) cx |= 0xF000;
        int16_t cy = (((int16_t)accBuffer[2] << 8) | accBuffer[3]) >> 4;
        if (cy & 0x0800) cy |= 0xF000;
        int16_t cz = (((int16_t)accBuffer[4] << 8) | accBuffer[5]) >> 4;
        if (cz & 0x0800) cz |= 0xF000;

        // Take Z offset into account!
        cz += zoffset;

        // Compute acceleration and temperature average over the last x readings
        outAccel->x = (float)cx * SENSITIVITY;
        outAccel->y = (float)cy * SENSITIVITY;
        outAccel->z = (float)cz * SENSITIVITY;
    }

	void dataInterruptHandler(uint32_t pin, nrf_gpiote_polarity_t action) {
		// pin and action don't matter

        // Read accelerometer data
        uint8_t accBuffer[6];
        I2C::readRegisters(devAddress, XOUT_H, accBuffer, 6);

        // Convert acc
        int16_t cx = (((int16_t)accBuffer[0] << 8) | accBuffer[1]) >> 4;
        if (cx & 0x0800) cx |= 0xF000;
        int16_t cy = (((int16_t)accBuffer[2] << 8) | accBuffer[3]) >> 4;
        if (cy & 0x0800) cy |= 0xF000;
        int16_t cz = (((int16_t)accBuffer[4] << 8) | accBuffer[5]) >> 4;
        if (cz & 0x0800) cz |= 0xF000;

        // Take Z offset into account!
        cz += zoffset;

        // Convert temperature
        int16_t ct = accBuffer[6];
        if (ct & 0x80) ct |= 0xFF00;  // Padding sign bits if negative

        if (sums.add(cx, cy, cz, ct)) {
            // Trigger the callbacks
            Scheduler::push(&sums, sizeof(AccelerometerAccumulator), [](void* sumsCopyPtr, uint16_t event_size) {

                // Cast the param to the right type
                auto sumsCopy = (AccelerometerAccumulator*)(sumsCopyPtr);

                // Compute acceleration and temperature average over the last x readings
                Core::float3 acc;
                acc.x = (float)sumsCopy->xSum * SENSITIVITY / sumsCopy->count;
                acc.y = (float)sumsCopy->ySum * SENSITIVITY / sumsCopy->count;
                acc.z = (float)sumsCopy->zSum * SENSITIVITY / sumsCopy->count;
                //float temp = ((float)sumsCopy->tSum * T_SENSITIVITY / sumsCopy->count) + T_ZERO;

				for (int i = 0; i < clients.Count(); ++i) {
					clients[i].handler(clients[i].token, acc);
				}
            });

            // Restart the averaging, the values were copied when passed to the scheduler
            sums.clear();
        }        
        
        clearInterrupt();
	}

    void reset() {
		// Trigger interrupt on data ready
		I2C::writeRegister(devAddress, INT_CLR1, CMD_RESET);
    }

	/// <summary>
	/// ENABLE INTERRUPT ON TRANSIENT MOTION DETECTION
	/// This function sets up the MMA8452Q to trigger an interrupt on pin 1
	/// when it detects any motion (lowest detectable threshold).
	/// </summary>
	void enableDataInterrupt() {
        // Set interrupt mask, clear all shake and orientation
		I2C::writeRegister(devAddress, INT_MASK0, 0x00);

		// Trigger interrupt on data ready
		I2C::writeRegister(devAddress, INT_MASK1, 0b00000001);

		// Set interrupt pin
		GPIOTE::enableInterrupt(
			BoardManager::getBoard()->accInterruptPin,
			NRF_GPIO_PIN_PULLUP,
			NRF_GPIOTE_POLARITY_HITOLO,
			dataInterruptHandler);
	}

    void disableDataInterrupt()
    {
        // Disable interrupt pin
		GPIOTE::disableInterrupt(BoardManager::getBoard()->accInterruptPin);

        // Disable interrupt on data ready
		I2C::writeRegister(devAddress, INT_MASK1, 0b00000000);
    }

    void clearDataInterrupt() {
        I2C::writeRegister(devAddress, INT_CLR1, 0b00000001);
    }

	/// <summary>
	/// ENABLE INTERRUPT ON TRANSIENT MOTION DETECTION
	/// This function sets up the MMA8452Q to trigger an interrupt on pin 1
	/// when it detects any motion (lowest detectable threshold).
	/// </summary>
	void enableInterrupt() {
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
	void clearInterrupt() {
        uint8_t src0 = I2C::readRegister(devAddress, INT_SRC0);
		I2C::writeRegister(devAddress, INT_CLR0, src0);

        uint8_t src1 = I2C::readRegister(devAddress, INT_SRC1);
		I2C::writeRegister(devAddress, INT_CLR1, src1);
	}

	/// <summary>
	/// DISABLE TRANSIENT INTERRUPT
	/// </summary>
	void disableInterrupt() {
        // Clear interrupt mask
		I2C::writeRegister(devAddress, INT_MASK0, 0x00);
	}

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hook(AccelClientMethod method, void* parameter)
	{
		if (!clients.Register(parameter, method))
		{
			NRF_LOG_ERROR("Too many MXC4005XC hooks registered.");
		}
	}

	/// <summary>
	/// Method used by clients to stop getting accelerometer reading callbacks
	/// </summary>
	void unHook(AccelClientMethod method)
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

    /*********************************************************************************
    * decription: return the absolute value
    *********************************************************************************/
    static float AbsValue(float value) {
        if(value < 0){
            return -value;
        }else{
            return value;  
        }
    }

    /*********************************************************************************
    * decription: set aoz value
    *********************************************************************************/
    void SetAozPara(float offset) {
        uint8_t reg_val[4];
        
        /* read reg 0x23-0x26 to get the initial aoz value and agz value */
        I2C::writeRegister(devAddress, PASSWORD, CMD_PASSWORD);
        I2C::readRegisters(devAddress, AOZL, reg_val, 4);
        
        // reg0 [aoz|aoz|aoz|aoz|aoz|aoz|x  |x  ]
        // reg1 [x  |x  |x  |x  |x  |aoz|aoz|aoz]
        int16_t sAOZ_Init = ((reg_val[1]&0x07)<<6) | ((reg_val[0]&0xfc)>>2);
        // reg2 [agz|agz|agz|x  |x  |x  |x  |x  ]
        // reg3 [x  |x  |x  |x  |x  |x  |aoz|aoz]
        int16_t sAGZ_value = ((reg_val[3]&0x03)<<3) | ((reg_val[2]&0xe0)>>5); 
        if (sAOZ_Init>255)
            sAOZ_Init -= 512;

        if (sAGZ_value>15)
            sAGZ_value -= 32;

        NRF_LOG_INFO("offset: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(offset));
        NRF_LOG_INFO("AOZ: %d, AGZ: %d", sAOZ_Init, sAGZ_value);

        /* get final aoz value*/
        float fAOZ_Delta = (offset*16)/((float)(1.0f + (float)sAGZ_value*0.03125f));
        NRF_LOG_INFO("fAOZ Delta: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(fAOZ_Delta));
        
        /* fAOZ_Delta rounding */
        int16_t sAOZ_Delta = 0;
        if (fAOZ_Delta > 0) {
            sAOZ_Delta = (int16_t)((fAOZ_Delta *10 + 5)/10);
        } else if (fAOZ_Delta < 0) {
            sAOZ_Delta = (int16_t)((fAOZ_Delta *10 - 5)/10);
        }

        NRF_LOG_INFO("sAOZ Delta: %d", sAOZ_Delta);

        int16_t sAOZ_Term = sAOZ_Init - sAOZ_Delta;
        if (sAOZ_Term < 0)
            sAOZ_Term += 512;

        NRF_LOG_INFO("New AOZ: %d", sAOZ_Term);

        int16_t sAGZ_Term = 0;
        NRF_LOG_INFO("New AGZ: %d", sAGZ_Term);

        // reg0 [aoz|aoz|aoz|aoz|aoz|aoz|x  |x  ]  [x  |x  |x  |x  |x  |aoz|aoz|aoz]
        uint8_t aoz_low  = ((sAOZ_Term & 0x3f) << 2) | (reg_val[0] & 0x03);
        uint8_t aoz_high = (reg_val[1] & 0xf8) | ((sAOZ_Term & 0x1c0) >> 6);

        // reg2 [agz|agz|agz|x  |x  |x  |x  |x  ]  [x  |x  |x  |x  |x  |x  |agz|agz]
        uint8_t agz_low  = ((sAGZ_Term & 0x7) << 5) | (reg_val[2] & 0x1F);
        uint8_t agz_high = (reg_val[3] & 0xfC) | ((sAGZ_Term & 0x1c0) >> 3);

        /* write the new aoz value to register 0x23 and 0x24 */
        I2C::writeRegister(devAddress, AOZL, aoz_low);
        I2C::writeRegister(devAddress, AOZH, aoz_high);
        I2C::writeRegister(devAddress, AGZL, agz_low);
        I2C::writeRegister(devAddress, AGZH, agz_high);


        return;	
    }


    /*********************************************************************************
    * decription: handle the aoz register
    *********************************************************************************/
    int AccAozHandle(float tmpr,float zoff, int z_dir) {
        static float preTmpr = 0.0;

        if(AbsValue(tmpr-25)>25){
            return -1;	
        }else{
            if(AbsValue(tmpr-25) > AbsValue(preTmpr-25)){
                return 0;
            }else{
                preTmpr = tmpr;
                zoff = z_dir * zoff;	
                
                SetAozPara(zoff);			
            }		
        }
        return 1;
    }



}
}

