#pragma once

#include <stdint.h>
#include <stddef.h>

namespace DriversNRF
{
    /// <summary>
    /// Wrapper for the Wire library that is set to use the Die pins
    /// </summary>
    namespace A2D
    {
        void init();

        int32_t readVBatTimes1000();
        int32_t read5VTimes1000();
        int32_t readVBoardTimes1000();
        int32_t readVDDTimes1000();
        int32_t readVNTCTimes1000();

        void selfTest();
        void selfTestBatt();
    }
}

