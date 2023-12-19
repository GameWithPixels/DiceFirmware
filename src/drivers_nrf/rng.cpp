#include "rng.h"
#include "config/board_config.h"
#include "log.h"
#include "nrf_sdh_soc.h"
#include "app_error.h"

namespace DriversNRF::RNG
{
    void init() {
        NRF_LOG_DEBUG("RNG init");
    }

    uint8_t randomVectorGenerate(uint8_t * p_buff, uint8_t size) {
        uint32_t err_code;
        uint8_t  available;

        sd_rand_application_bytes_available_get(&available);
        uint8_t length = MIN(size, available);

        err_code = sd_rand_application_vector_get(p_buff, length);
        APP_ERROR_CHECK(err_code);

        return length;
    }

    uint8_t randomUInt8() {
        uint8_t ret;
        randomVectorGenerate(&ret, sizeof(ret));
        return ret;
    }

    uint16_t randomUInt16() {
        uint16_t ret;
        randomVectorGenerate((uint8_t*)(void*)&ret, sizeof(ret));
        return ret;
    }

    uint32_t randomUInt32() {
        uint32_t ret;
        randomVectorGenerate((uint8_t*)(void*)&ret, sizeof(ret));
        return ret;
    }

}
