#include "i2c.h"
#include "nrf_drv_twi.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "drivers_nrf/gpiote.h"

namespace DriversNRF
{
namespace I2C
{
    /* TWI instance. */
    static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);

    // Test
    void scanBus(); 


    void init()
    {
        auto board = Config::BoardManager::getBoard();

        nrf_drv_twi_frequency_t freq = NRF_DRV_TWI_FREQ_100K;
        if (board->accModel == Config::AccelerometerModel::MXC4005XC) {
            freq = NRF_DRV_TWI_FREQ_400K;
        }

        const nrf_drv_twi_config_t twi_config = {
        .scl                = board->i2cClockPin,
        .sda                = board->i2cDataPin,
        .frequency          = freq,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init     = false
        };

        auto err = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
        if (err != NRF_SUCCESS) {
            NRF_LOG_ERROR("I2C Initialization Failed, err=0x%x", err);
        }

        // Enable the interface
        nrf_drv_twi_enable(&m_twi);

        //scanBus();

        NRF_LOG_INFO("I2C Initialized with freq=%d", (int)freq);
    }

    bool write(uint8_t device, uint8_t value, bool no_stop)
    {
        return write(device, &value, 1, no_stop);
    }

    bool write(uint8_t device, const uint8_t* data, size_t size, bool no_stop)
    {
        auto err = nrf_drv_twi_tx(&m_twi, device, data, size, no_stop);
        if (err != NRF_SUCCESS) {
            NRF_LOG_ERROR("I2C Write Error 0x%x", err);
        }
        return err == NRF_SUCCESS;
    }

    bool read(uint8_t device, uint8_t* value)
    {
        return read(device, value, 1);
    }

    bool read(uint8_t device, uint8_t* data, size_t size)
    {
        auto err = nrf_drv_twi_rx(&m_twi, device, data, size);
        if (err != NRF_SUCCESS) {
            NRF_LOG_ERROR("I2C Read Error 0x%x", err);
        }
        return err == NRF_SUCCESS;
    }

	/// <summary>
	/// WRITE A SINGLE REGISTER
	/// Write a single uint8_t of data to a register in the MMA8452Q.
	/// </summary>
	void writeRegister(uint8_t device, uint8_t reg, uint8_t data)
	{
		uint8_t bytes[2];
		bytes[0] = reg;
		bytes[1] = data;
		write(device, bytes, 2);
	}

	/// <summary>
	/// READ A SINGLE REGISTER
	///	Read a uint8_t from the device register "reg".
	/// </summary>
	uint8_t readRegister(uint8_t device, uint8_t reg)
	{
		write(device, reg, true);
		uint8_t ret = 0;
		read(device, &ret, 1);
		return ret;
	}

	/// <summary>
	/// READ MULTIPLE REGISTERS
	///	Read "len" bytes from the device, starting at register "reg". Bytes are stored
	///	in "buffer" on exit.
	/// </summary>
	void readRegisters(uint8_t device, uint8_t reg, uint8_t *buffer, uint8_t len)
	{
		write(device, reg, true);
		read(device, buffer, len);
	}

	/// <summary>
	/// READ 16-bit register
	///	Read "len" bytes from the device, starting at register "reg". Bytes are stored
	///	in "buffer" on exit.
	/// </summary>
    uint16_t readRegisterInt16(uint8_t device, uint8_t reg)
    {
        //offset |= 0x80; //turn auto-increment bit on
        uint8_t myBuffer[2];
        readRegisters(device, reg, myBuffer, 2);  //Does memory transfer
        int16_t output = (int16_t)myBuffer[0] | int16_t(myBuffer[1] << 8);
        return output;
    }

    void scanBus() {
        ret_code_t err_code;
        uint8_t address;
        uint8_t sample_data;

        for (;;) {
            bool detected_device = false;

            for (address = 1; address <= 127; address++)
            {
                NRF_LOG_INFO("Testing address 0x%x.", address);
                err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, sizeof(sample_data));
                if (err_code == NRF_SUCCESS)
                {
                    detected_device = true;
                    NRF_LOG_INFO("I2C device detected at address 0x%x.", address);
                }
            }

            if (!detected_device)
            {
                NRF_LOG_INFO("No device was found.");
            }
        }
    }

}
}




