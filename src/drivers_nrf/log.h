#pragma once

namespace DriversNRF
{
    // Initializes the sdk log system
    namespace Log
    {
        void init();
        bool process();
        bool hasKey();
        int getKey();
        void selfTest();
    }
}
