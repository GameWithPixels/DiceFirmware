#include <stdint.h>

namespace Config::ValueStore
{
    enum ValueType : uint8_t
    {
        ValueType_ColorAndDesign = 1,
        ValueType_ValidationStartIndex = 0xA0, // Start index for validation value types, even value = timestamp, odd = geolocation
    };

    enum WriteValueError
    {
        WriteValueError_StoreFull = -1,
        WriteValueError_NotPermited = -2,
    };

    // Write value to store, returns its index (or -1 if full)
    int writeValue(uint32_t value);
    // Read value from store for the given type, returns -1 if not found
    uint32_t readValue(ValueType type);
}
