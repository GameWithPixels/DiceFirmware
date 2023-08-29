#pragma once

#include "profile_data.h"

namespace Profile::Static
{
    typedef void (*InitCallback)();

    void init(InitCallback callback);
    bool CheckValid();
    bool refreshData();

    uint32_t availableDataSize();
    uint32_t getSize();
    uint32_t getHash();
    const Data* getData();
}

