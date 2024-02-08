#include "value_store.h"
#include "drivers_nrf/log.h"
#include "modules/validation_manager.h"
#include "nrf_nvmc.h"

#define INDEX_RBEGIN (sizeof(NRF_UICR->CUSTOMER) / 4 - 1) // Index of "reverse" begin (higher value)
#define INDEX_REND 1                                      // Index of "reverse" end (lower value)

using namespace Modules;

namespace Config::ValueStore
{
    int writeValue(uint32_t value) {
        if (!ValidationManager::inValidation()) {
            return WriteValueError_NotPermited;
        }

        // Search for an empty slot in UICR registers.
        // This works similarly to heap v.s. stack:
        // - other setting stored in UICR are written to registers with low indices,
        // - stored values are written to registers with high indices
        // To write a new value, we search for the first empty register starting with
        // highest index and going down. We don't write to the first register
        // which is reserved for settings (even if empty).
        for (int i = INDEX_RBEGIN; i >= INDEX_REND; --i) {
            uint32_t *reg = (uint32_t *)&NRF_UICR->CUSTOMER[i];
            NRF_LOG_DEBUG("Read UICR[%d] => %x", i, *reg); 
            if (*reg == 0xffffffff) {
                NRF_LOG_DEBUG("Writing %x to UICR[%d]", value, i);
                nrf_nvmc_write_word((uint32_t)&NRF_UICR->CUSTOMER[i], value);
                return i;
            }
        }
        return WriteValueError_StoreFull;
    }

    uint32_t readValue(ValueType type) {
        uint32_t value = -1;
        const uint32_t typeMask = (uint32_t)type << 24;
        // Iterate through all values and keep the last one that has the required type
        for (int i = INDEX_RBEGIN; i >= INDEX_REND; --i) {
            uint32_t *reg = (uint32_t *)&NRF_UICR->CUSTOMER[i];
            NRF_LOG_DEBUG("Read UICR[%d] => %x", i, *reg);
            if (*reg == 0xffffffff) {
                // We reached the on the store
                break;
            }
            if ((*reg & 0xff000000) == typeMask) {
                value = *reg & 0xffffff;
            }
        }
        return value;
    }
}