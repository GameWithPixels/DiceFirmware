#pragma once

#include <stdint.h>
#include <stddef.h>

namespace DriversNRF
{
    /// <summary>
    /// Wrapper for the Wire library that is set to use the Die pins
    /// </summary>
    namespace I2C
    {
        void init();
        void deinit();

        bool write(uint8_t device, uint8_t value, bool no_stop = false);
        bool write(uint8_t device, const uint8_t* data, size_t size, bool no_stop = false);
        bool read(uint8_t device, uint8_t* value);
        bool read(uint8_t device, uint8_t* data, size_t size);

        void writeRegister(uint8_t device, uint8_t reg, uint8_t data);
        uint8_t readRegister(uint8_t device, uint8_t reg);
        void readRegisters(uint8_t device, uint8_t reg, uint8_t *buffer, uint8_t len);
        int16_t readRegisterInt16(uint8_t device, uint8_t reg);
        uint16_t readRegisterUInt16(uint8_t device, uint8_t reg);
    }
}

