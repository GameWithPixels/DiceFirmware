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
    enum qma7981_full_scale_range_t
    {
        RANGE_2G = 0b0001,
        RANGE_4G = 0b0010,
        RANGE_8G = 0b0100,
        RANGE_16G = 0b1000,
        RANGE_32G = 0b1111
    };

    enum qma7981_bandwidth_t
    {
        MCLK_DIV_BY_7695 = 0b000,
        MCLK_DIV_BY_3855 = 0b001,
        MCLK_DIV_BY_1935 = 0b010,
        MCLK_DIV_BY_975 = 0b011,
        MCLK_DIV_BY_15375 = 0b101,
        MCLK_DIV_BY_30735 = 0b110,
        MCLK_DIV_BY_61455 = 0b111
    };

    enum qma7981_clock_freq_t
    {
        CLK_500_KHZ = 0b0001,
        CLK_333_KHZ = 0b0000,
        CLK_200_KHZ = 0b0010,
        CLK_100_KHZ = 0b0011,
        CLK_50_KHZ = 0b0100,
        CLK_25_KHZ = 0b0101,
        CLK_12_KHZ_5 = 0b0110,
        CLK_5_KHZ = 0b0111
    };

    enum qma7981_no_motion_duration_t
    {
        NO_MOTION_1_SEC = 0b000000,
        NO_MOTION_2_SEC = 0b000001,
        NO_MOTION_3_SEC = 0b000010,
        NO_MOTION_5_SEC = 0b000100,
        NO_MOTION_10_SEC = 0b001001,
        NO_MOTION_15_SEC = 0b001110,
        NO_MOTION_30_SEC = 0b010010,
        NO_MOTION_1_MIN = 0b011000,
        NO_MOTION_2_MIN = 0b100010,
        NO_MOTION_3_MIN = 0b101000,
        NO_MOTION_4_MIN = 0b101110
    };

    enum qma7981_any_motion_samples_t
    {
        NUM_SAMPLES_1 = 0b00,
        NUM_SAMPLES_2 = 0b01,
        NUM_SAMPLES_3 = 0b10,
        NUM_SAMPLES_4 = 0b11
    };

    enum qma7981_mode_t
    {
        MODE_STANDBY = 0,
        MODE_ACTIVE = 1
    };

    enum qma7981_motion_detect_t
    {
        MOTION_DETECT_NOTHING = 0,
        MOTION_DETECT_ANY_MOTION = 1,
        MOTION_DETECT_NO_MOTION = 2
    };


    #define I2C_ADDRESS 0b0010010 // Device address when ADO = 0
	
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

        NRF_LOG_DEBUG("KXTJ3 init");
    }

    void read(Core::float3* outAccel) {

        uint16_t cx, cy, cz;
        cx = I2C::readRegisterInt16(devAddress, OUT_X_L);
        cy = I2C::readRegisterInt16(devAddress, OUT_Y_L);
        cz = I2C::readRegisterInt16(devAddress, OUT_Z_L);
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

void soft_reset()
{
    write_byte(0x36, 0xB6);
    write_byte(0x36, 0x00);
}

void set_interrupt_all_latch(bool latch)
{
    uint8_t data = read_byte(0x21);
    set_bit(&data, 0, latch);
    set_bit(&data, 7, 1); // clear all interrupts after reading any interrupt status register
    write_byte(0x21, data);
}

void reset_motion_detector(bool any_motion, bool significant_motion, bool no_motion)
{
    // to reset, write 0 first
    uint8_t data = read_byte(0x30);
    set_bit(&data, 2, !no_motion);
    set_bit(&data, 1, !significant_motion);
    set_bit(&data, 0, !any_motion);
    write_byte(0x30, data);
    // then write 1
    set_bit(&data, 2, 1);
    set_bit(&data, 1, 1);
    set_bit(&data, 0, 1);
    write_byte(0x30, data);
}

void set_any_motion_axis(bool x_enabled, bool y_enabled, bool z_enabled)
{
    uint8_t data = read_byte(0x18);
    set_bit(&data, 0, x_enabled);
    set_bit(&data, 1, y_enabled);
    set_bit(&data, 2, z_enabled);
    write_byte(0x18, data);
}

void set_any_motion_number_of_samples(qma7981_any_motion_samples_t samples)
{
    uint8_t data = read_byte(0x2C);
    data &= 0b11111100; // clear bits 1-0
    data |= (samples & 0b11);
    write_byte(0x2C, data);
}

void set_any_motion_threshold(uint8_t threshold)
{
    write_byte(0x2E, threshold);
}

void set_no_motion_axis(bool x_enabled, bool y_enabled, bool z_enabled)
{
    uint8_t data = read_byte(0x18);
    set_bit(&data, 5, x_enabled);
    set_bit(&data, 6, y_enabled);
    set_bit(&data, 7, z_enabled);
    write_byte(0x18, data);
}

void set_no_motion_duration(qma7981_no_motion_duration_t duration)
{
    uint8_t data = read_byte(0x2C);
    data &= 0b00000011; // clear bits 7-2
    data |= ((duration & 0b111111) << 2);
    write_byte(0x2C, data);
}

void set_no_motion_threshold(uint8_t threshold)
{
    write_byte(0x2D, threshold);
}

void set_any_or_significant_motion(bool significant_motion)
{
    uint8_t data = read_byte(0x2F);
    set_bit(&data, 0, significant_motion);
    write_byte(0x2F, data);
}

// public functions

QMA7981::QMA7981()
{
}

void QMA7981::initialize_default()
{
    delay(10);
    soft_reset();
    delay(10);
    set_mode(MODE_ACTIVE);                  // bring out of sleep mode
    set_clock_freq(CLK_50_KHZ);             // set digital clock freq
    set_bandwidth(MCLK_DIV_BY_61455);       // set bandwitch (samples per sec)
    set_full_scale_range(RANGE_2G);         // set full scale acceleration range
    set_interrupt_all_latch(true);          // set interrupt pin to latch after interrupt until interrupt status read
    set_interrupt_pin_1_type(false, false); // set interrupt pin type and logic level
    // enable no motion and any motion interrupts to trigger int pin 1
    set_interrupt_pin_1_source(false, false, false, false, true, true, false, true);
}

int16_t QMA7981::get_accel_x()
{
    return read_accel_axis(0x01);
}

int16_t QMA7981::get_accel_y()
{
    return read_accel_axis(0x03);
}

int16_t QMA7981::get_accel_z()
{
    return read_accel_axis(0x05);
}

uint8_t QMA7981::get_chip_id()
{
    return read_byte(0x00);
}

void QMA7981::set_full_scale_range(qma7981_full_scale_range_t range)
{
    uint8_t data = 0b11110000;
    data |= (range & 0b1111);
    write_byte(0x0F, data);
}

void QMA7981::set_bandwidth(qma7981_bandwidth_t bandwidth)
{
    uint8_t data = 0b11100000;
    data |= (bandwidth & 0b111);
    write_byte(0x10, data);
}

void QMA7981::set_clock_freq(qma7981_clock_freq_t freq)
{
    uint8_t data = read_byte(0x11);
    // TODO T_RSTB_SINC_SEL<1:0, right now kept at default of 0
    data &= 0b11110000;      // clear bits 0-3
    data |= (freq & 0b1111); // set freq on bits 0-3
    write_byte(0x11, data);
}

void QMA7981::set_mode(qma7981_mode_t mode)
{
    uint8_t data = read_byte(0x11);
    set_bit(&data, 7, mode);
    write_byte(0x11, data);
}

void QMA7981::set_interrupt_pin_1_source(bool significant_step, bool step_valid, bool hand_down,
                                         bool hand_raise, bool significant_motion,
                                         bool any_motion, bool data_ready, bool no_motion)
{
    uint8_t data = read_byte(0x19);
    set_bit(&data, 0, significant_motion);
    set_bit(&data, 1, hand_raise);
    set_bit(&data, 2, hand_down);
    set_bit(&data, 3, step_valid);
    set_bit(&data, 6, significant_step);
    write_byte(0x19, data);

    data = read_byte(0x1A);
    set_bit(&data, 0, any_motion);
    set_bit(&data, 4, data_ready);
    set_bit(&data, 7, no_motion);
    write_byte(0x1A, data);
}

void QMA7981::set_interrupt_pin_1_type(bool open_drain, bool active_high)
{
    uint8_t data = read_byte(0x20);
    set_bit(&data, 0, active_high);
    set_bit(&data, 1, open_drain);
    write_byte(0x20, data);
}

void QMA7981::setup_any_motion_detector(bool x_enabled, bool y_enabled, bool z_enabled,
                                        qma7981_any_motion_samples_t samples,
                                        uint8_t threshold)
{
    set_any_motion_axis(x_enabled, y_enabled, z_enabled);
    set_any_motion_number_of_samples(samples);
    set_any_motion_threshold(threshold);
    set_any_or_significant_motion(false);
    reset_motion_detector(true, false, false);
}

void QMA7981::setup_no_motion_detector(bool x_enabled, bool y_enabled, bool z_enabled,
                                       qma7981_no_motion_duration_t duration,
                                       uint8_t threshold)
{
    set_no_motion_axis(x_enabled, y_enabled, z_enabled);
    set_no_motion_duration(duration);
    set_no_motion_threshold(threshold);
    reset_motion_detector(false, false, true);
}

qma7981_motion_detect_t QMA7981::get_motion_detected()
{
    uint8_t data = read_byte(0x09);

    bool no_motion = get_bit(data, 7);
    if (no_motion)
    {
        return MOTION_DETECT_NO_MOTION;
    }

    bool x_any_motion = get_bit(data, 2);
    bool y_any_motion = get_bit(data, 1);
    bool z_any_motion = get_bit(data, 0);
    if (x_any_motion || y_any_motion || z_any_motion)
    {
        return MOTION_DETECT_ANY_MOTION;
    }

    return MOTION_DETECT_NOTHING;
}

void QMA7981::disable_any_motion_detector()
{
    set_any_motion_axis(false, false, false); // disable all 3 any motion detect axis
}

void QMA7981::disable_no_motion_detector()
{
    set_no_motion_axis(false, false, false); // disable all 3 any motion detect axis
}

void qma7981_setup_default()
{
}

// TODO: add interrupt pin 2 set
// TODO: add function to get axis and direction triggering any motion interrupt
// TODO: add function to get step interrupt status
// TODO: add function to get hand raise interrupt status
// TODO: add function to get data ready interrupt status