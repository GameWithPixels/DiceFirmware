#include "pixel.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_error.h"
#include "value_store.h"
#include "hardfault/hardfault.h"

#if !defined(BUILD_TIMESTAMP)
    #warning Build timestamp not defined
    #define BUILD_TIMESTAMP 0
#endif

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    NRF_LOG_ERROR("app_error_handler err_code:%d %s:%d", error_code, p_file_name, line_num);
    on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}

void app_error_handler_bare(ret_code_t error_code) {
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

void HardFault_process(HardFault_stack_t *p_stack) {
    // Error already logged in default implementation of HardFault_c_handler
    on_error();
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

namespace Pixel
{
    uint32_t getDeviceID() {
        return NRF_FICR->DEVICEID[1] ^ NRF_FICR->DEVICEID[0];
    }

    uint32_t getBuildTimestamp() {
        return BUILD_TIMESTAMP;
    }

    RunMode getCurrentRunMode() {
        // -1 (no value stored) should be handled correctly and be cast to RunMode_Invalid
        return (RunMode)Config::ValueStore::readValue(Config::ValueStore::ValueType_RunMode);
    }

    bool setCurrentRunMode(RunMode mode) {
        int index = Config::ValueStore::writeValue(Config::ValueStore::ValueType_RunMode, mode);
        return index != -1;
    }
}