#include "scheduler.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "app_timer.h"
#include "nrf_log.h"

#define SCHED_MAX_EVENT_DATA_SIZE      16        /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE               16        /**< Maximum number of events in the scheduler queue. */

namespace DriversNRF::Scheduler
{
    void init() {
        // Macro expansion calls APP_ERROR_CHECK automatically
        APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
        NRF_LOG_INFO("Scheduler: %dB free", app_sched_queue_space_get() * SCHED_MAX_EVENT_DATA_SIZE);
    }

    void update() {
        app_sched_execute();
    }

    bool push(const void* eventData, uint16_t size, app_sched_event_handler_t handler) {
        ASSERT(size <= SCHED_MAX_EVENT_DATA_SIZE);
        ret_code_t ret = app_sched_event_put((void*)eventData, size, handler);
        if (ret != NRF_SUCCESS) {
            NRF_LOG_ERROR("Scheduler push failed, error %s(0x%x)", NRF_LOG_ERROR_STRING_GET(ret), ret);
        }
        return ret == NRF_SUCCESS;
    }
}
