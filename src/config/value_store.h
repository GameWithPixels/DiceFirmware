#include <stdint.h>

namespace Config::ValueStore
{
    enum ValueType : uint8_t
    {
        ValueType_None = 0,
        ValueType_DieType = 1,
        ValueType_Colorway = 2,
        ValueType_RunMode = 3,
        ValueType_ValidationTimestampStart = 0xA0, // Start index for validation timestamps
        ValueType_ValidationTimestampFirmware = ValueType_ValidationTimestampStart,
        ValueType_ValidationTimestampBoardNoCoil,
        ValueType_ValidationTimestampBoard,
        ValueType_ValidationTimestampDie,
        ValueType_ValidationTimestampDieFinal,
        ValueType_ValidationTimestampEnd = 0xFF,
    };

    enum WriteValueError
    {
        WriteValueError_StoreFull = -1,
        WriteValueError_NotPermited = -2,
    };

    // Write value to store, returns its index (or -1 if full)
    int writeUInt32(uint32_t value);

    // Write specific type of value, returns its index (or -1 if full)
    int writeValue(ValueType type, uint32_t value);

    // Read value from store for the given type, returns -1 if not found
    uint32_t readValue(ValueType typeStart, ValueType typeEnd = ValueType_None);

    // Check if the board has been validated at least once
    bool hasValidationTimestamp();
}
